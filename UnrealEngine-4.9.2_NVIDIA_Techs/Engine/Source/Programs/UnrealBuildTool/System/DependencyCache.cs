// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Formatters.Binary;

namespace UnrealBuildTool
{
	[Serializable]
	public class DependencyInclude
	{
		/// <summary>
		/// Public ctor that initializes the include name (the resolved name won't be determined until later)
		/// </summary>
		/// <param name="InIncludeName"></param>
		public DependencyInclude(string InIncludeName)
		{
			IncludeName = InIncludeName;
		}

		/// <summary>
		/// These are direct include paths and cannot be resolved to an actual file on disk without using the proper list of include directories for this file's module 
		/// </summary>
		public readonly string IncludeName;
		/// <summary>
		/// This is the fully resolved name. We can't really store this globally, but we're going to see how well it works.
		/// </summary>
		public string IncludeResolvedName;

		public override string ToString()
		{
			return IncludeName;
		}
	}

	[Serializable]
	struct DependencyInfo
	{
		/** List of files this file includes.  These are direct include paths and cannot be resolved to an actual file on 
		    disk without using the proper list of include directories for this file's module */
		public List<DependencyInclude> Includes;

		/** Deprecated - always false */
		public bool HasUObjects;
	}


	/**
	 * Caches include dependency information to speed up preprocessing on subsequent runs.
	 */
	[Serializable]
	public class DependencyCache
	{
		/** The time the cache was created. Used to invalidate entries. */
		public DateTimeOffset CacheCreateDate;

		/** The time the cache was last updated. Stored as the creation date when saved. */
		[NonSerialized]
		private DateTimeOffset CacheUpdateDate;

		/** Path to store the cache data to. */
		[NonSerialized]
		private string CachePath;

		/** Dependency lists, keyed (case-insensitively) on file's absolute path. */
		private Dictionary<string, DependencyInfo> DependencyMap;

		[NonSerialized]
		private Dictionary<string, bool> FileExistsInfo;

		/** Whether the dependency cache is dirty and needs to be saved. */
		[NonSerialized]
		private bool bIsDirty;

		/**
		 * Creates and deserializes the dependency cache at the passed in location
		 * 
		 * @param	CachePath	Name of the cache file to deserialize
		 */
		public static DependencyCache Create(string CachePath)
		{
			// See whether the cache file exists.
			FileItem Cache = FileItem.GetItemByPath(CachePath);
			if (Cache.bExists)
			{
				if (BuildConfiguration.bPrintPerformanceInfo)
				{
					Log.TraceInformation("Loading existing IncludeFileCache: " + Cache.AbsolutePath);
				}

				var TimerStartTime = DateTime.UtcNow;

				// Deserialize cache from disk if there is one.
				DependencyCache Result = Load(Cache);
				if (Result != null)
				{
					// Successfully serialize, create the transient variables and return cache.
					Result.CachePath = CachePath;
					Result.CacheUpdateDate = DateTimeOffset.Now;

					var TimerDuration = DateTime.UtcNow - TimerStartTime;
					if (BuildConfiguration.bPrintPerformanceInfo)
					{
						Log.TraceInformation("Loading IncludeFileCache took " + TimerDuration.TotalSeconds + "s");
					}
					Telemetry.SendEvent("LoadIncludeDependencyCacheStats.2",
						"TotalDuration", TimerDuration.TotalSeconds.ToString("0.00"));
					return Result;
				}
			}
			// Fall back to a clean cache on error or non-existance.
			return new DependencyCache(Cache);
		}

		/**
		 * Loads the cache from the passed in file.
		 * 
		 * @param	Cache	File to deserialize from
		 */
		public static DependencyCache Load(FileItem Cache)
		{
			DependencyCache Result = null;
			try
			{
				string CacheBuildMutexPath = Cache.AbsolutePath + ".buildmutex";

				// If the .buildmutex file for the cache is present, it means that something went wrong between loading
				// and saving the cache last time (most likely the UBT process being terminated), so we don't want to load
				// it.
				if (!File.Exists(CacheBuildMutexPath))
				{
					using (File.Create(CacheBuildMutexPath))
					{
					}

					using (FileStream Stream = new FileStream(Cache.AbsolutePath, FileMode.Open, FileAccess.Read))
					{
						BinaryFormatter Formatter = new BinaryFormatter();
						Result = Formatter.Deserialize(Stream) as DependencyCache;
					}
					Result.CreateFileExistsInfo();
					Result.ResetUnresolvedDependencies();
				}
			}
			catch (Exception Ex)
			{
				// Don't bother logging this expected error.
				// It's due to a change in the CacheCreateDate type.
				if (Ex.Message != "Object of type 'System.DateTime' cannot be converted to type 'System.DateTimeOffset'" &&
	Ex.Message != "Object of type 'System.Collections.Generic.Dictionary`2[System.String,System.Collections.Generic.List`1[System.String]]' cannot be converted to type 'System.Collections.Generic.Dictionary`2[System.String,UnrealBuildTool.DependencyInfo]'.")	// To catch serialization differences added when we added the DependencyInfo struct
				{
					Console.Error.WriteLine("Failed to read dependency cache: {0}", Ex.Message);
				}
			}
			return Result;
		}

		/**
		 * Constructor
		 * 
		 * @param	Cache	File associated with this cache
		 */
		protected DependencyCache(FileItem Cache)
		{
			CacheCreateDate = DateTimeOffset.Now;
			CacheUpdateDate = DateTimeOffset.Now;
			CachePath = Cache.AbsolutePath;
			DependencyMap = new Dictionary<string, DependencyInfo>();
			bIsDirty = false;
			CreateFileExistsInfo();
		}

		/**
		 * Saves the dependency cache to disk using the update time as the creation time.
		 */
		public void Save()
		{
			// Only save if we've made changes to it since load.
			if (bIsDirty)
			{
				var TimerStartTime = DateTime.UtcNow;

				// Save update date as new creation date.
				CacheCreateDate = CacheUpdateDate;

				// Serialize the cache to disk.
				try
				{
					Directory.CreateDirectory(Path.GetDirectoryName(CachePath));
					using (FileStream Stream = new FileStream(CachePath, FileMode.Create, FileAccess.Write))
					{
						BinaryFormatter Formatter = new BinaryFormatter();
						Formatter.Serialize(Stream, this);
					}
				}
				catch (Exception Ex)
				{
					Console.Error.WriteLine("Failed to write dependency cache: {0}", Ex.Message);
				}

				if (BuildConfiguration.bPrintPerformanceInfo)
				{
					var TimerDuration = DateTime.UtcNow - TimerStartTime;
					Log.TraceInformation("Saving IncludeFileCache took " + TimerDuration.TotalSeconds + "s");
				}
			}
			else
			{
				if (BuildConfiguration.bPrintPerformanceInfo)
				{
					Log.TraceInformation("IncludeFileCache did not need to be saved (bIsDirty=false)");
				}
			}

			try
			{
				File.Delete(CachePath + ".buildmutex");
			}
			catch
			{
				// We don't care if we couldn't delete this file, as maybe it couldn't have been created in the first place.
			}
		}

		/**
		 * Returns the direct dependencies of the specified FileItem if it exists in the cache and they are not stale.
		 *
		 * @param  File  File to try to find dependencies in cache
		 * @returns  Dependency info for File, or null if no dependencies are cached or if the cache is stale.
		 */
		public List<DependencyInclude> GetCachedDependencyInfo(FileItem File)
		{
			string LowercaseFilePath = File.AbsolutePath.ToLowerInvariant();

			// Check whether File is in cache.
			DependencyInfo Result;
			if (!DependencyMap.TryGetValue(LowercaseFilePath, out Result))
			{
				return null;
			}

			// File is in cache, now check whether last write time is prior to cache creation time.
			if (File.LastWriteTime >= CacheCreateDate)
			{
				// Remove entry from cache as it's stale.
				DependencyMap.Remove(LowercaseFilePath);
				bIsDirty = true;
				return null;
			}

			// Check if any of the resolved includes is missing
			foreach (var Include in Result.Includes)
			{
				if (!String.IsNullOrEmpty(Include.IncludeResolvedName))
				{
					bool bIncludeExists = false;
					string LowercaseIncludePath = Include.IncludeResolvedName.ToLowerInvariant();
					if (!FileExistsInfo.TryGetValue(LowercaseIncludePath, out bIncludeExists))
					{
						bIncludeExists = System.IO.File.Exists(Include.IncludeResolvedName);
						FileExistsInfo.Add(LowercaseIncludePath, bIncludeExists);
					}

					if (!bIncludeExists)
					{
						// Remove entry from cache as it's stale, as well as the include which no longer exists
						DependencyMap.Remove(LowercaseIncludePath);
						DependencyMap.Remove(LowercaseFilePath);
						bIsDirty = true;
						return null;
					}
				}
			}

			// Cached version is up to date, return it.
			return Result.Includes;
		}

		/**
		 * Update cache with dependencies for the passed in file.
		 * 
		 * @param	File			File to update dependencies for
		 * @param	Dependencies	List of dependencies to cache for passed in file
		 * @param	HasUObjects		True if this file was found to contain UObject classes or types
		 */
		public void SetDependencyInfo(FileItem File, List<DependencyInclude> Info)
		{
			DependencyMap[File.AbsolutePath.ToLowerInvariant()] = new DependencyInfo{ Includes = Info, HasUObjects = false };
			bIsDirty = true;
		}

		/// <summary>
		/// Creates and pre-allocates a map for storing information about the physical presence of files on disk.
		/// </summary>
		private void CreateFileExistsInfo()
		{
			// Pre-allocate about 125% of the dependency map count (which amounts to 1.25 unique includes per dependency which is a little more than empirical
			// results show but gives us some slack in case something gets added).
			FileExistsInfo = new Dictionary<string, bool>((DependencyMap.Count * 5) / 4);
		}

		/// <summary>
		/// Resets unresolved dependency include files so that the compile environment can attempt to re-resolve them.
		/// </summary>
		public void ResetUnresolvedDependencies()
		{
			foreach (var Dependency in DependencyMap)
			{
				foreach (var Include in Dependency.Value.Includes)
				{
					if (Include.IncludeResolvedName == String.Empty)
					{
						Include.IncludeResolvedName = null;
					}
				}
			}
		}

		/// <summary>
		/// Caches the fully resolved path of the include.
		/// TODO: This method should be more tightly coupled with the Resolve step itself so we don't have to reach into the cache externally
		/// using internal details like the list index.
		/// </summary>
		/// <param name="File">The file whose include is being resolved</param>
		/// <param name="DirectlyIncludedFileNameIndex">Index in the resolve list to quickly find the include in question in the existing cache.</param>
		/// <param name="DirectlyIncludedFileNameFullPath">Full path name of the resolve include.</param>
		public void CacheResolvedIncludeFullPath(FileItem File, int DirectlyIncludedFileNameIndex, string DirectlyIncludedFileNameFullPath)
		{
			if (BuildConfiguration.bUseIncludeDependencyResolveCache)
			{
				var Includes = DependencyMap[File.AbsolutePath.ToLowerInvariant()].Includes;
				var IncludeToResolve = Includes[DirectlyIncludedFileNameIndex];
				if (BuildConfiguration.bTestIncludeDependencyResolveCache)
				{
					// test whether there are resolve conflicts between modules with different include paths.
					if (IncludeToResolve.IncludeResolvedName != null && IncludeToResolve.IncludeResolvedName != DirectlyIncludedFileNameFullPath)
					{
						throw new BuildException("Found directly included file that resolved differently in different modules. File ({0}) had previously resolved to ({1}) and now resolves to ({2}).",
							File.AbsolutePath, IncludeToResolve.IncludeResolvedName, DirectlyIncludedFileNameFullPath);
					}
				}
				Includes[DirectlyIncludedFileNameIndex].IncludeResolvedName = DirectlyIncludedFileNameFullPath;
				if (!String.IsNullOrEmpty(DirectlyIncludedFileNameFullPath))
				{
					bIsDirty = true;
				}
			}
		}

		/// <summary>
		/// Gets the dependency cache path and filename for the specified target.
		/// </summary>
		/// <param name="Target">Current build target</param>
		/// <returns>Cache Path</returns>
		public static string GetDependencyCachePathForTarget(UEBuildTarget Target)
		{
			string PlatformIntermediatePath = BuildConfiguration.PlatformIntermediatePath;
			if (UnrealBuildTool.HasUProjectFile())
			{
				PlatformIntermediatePath = Path.Combine(UnrealBuildTool.GetUProjectPath(), BuildConfiguration.PlatformIntermediateFolder);
			}
			string CachePath = Path.Combine(PlatformIntermediatePath, Target.GetTargetName(), "DependencyCache.bin");
			return CachePath;
		}
	}
}
