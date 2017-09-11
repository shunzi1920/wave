// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkeletalMeshTypes.h"
#include "BoneContainer.h"
#include "BoneIndices.h"
#include "CustomBoneIndexArray.h"
#include "AnimEncoding.h"


struct FCompactPose;

struct FBoneTransform
{
	/** @todo anim: should be Skeleton bone index in the future, but right now it's CompactBoneIndex **/
	FCompactPoseBoneIndex BoneIndex;

	/** Transform to apply **/
	FTransform Transform;

	FBoneTransform()
		: BoneIndex(INDEX_NONE)
	{}

	FBoneTransform(FCompactPoseBoneIndex InBoneIndex, const FTransform& InTransform)
		: BoneIndex(InBoneIndex)
		, Transform(InTransform)
	{}
};

template<class BoneIndexType>
struct FBasePose
{
public:
	FORCEINLINE void InitBones(int32 NumBones) { Bones.Empty(NumBones); Bones.AddUninitialized(NumBones); }

	FORCEINLINE int32 GetNumBones() const { return Bones.Num(); }

	FORCEINLINE bool IsValidIndex(const BoneIndexType& BoneIndex) const
	{
		return Bones.IsValidIndex(BoneIndex.GetInt());
	}

	FORCEINLINE FTransform& operator[](const BoneIndexType& BoneIndex)
	{ 
		return Bones[BoneIndex.GetInt()];
	}

	FORCEINLINE const FTransform& operator[] (const BoneIndexType& BoneIndex) const
	{
		return Bones[BoneIndex.GetInt()];
	}

	//Bone Index Iteration
	template<typename PoseType, typename IterType>
	struct FRangedForSupport
	{
		const PoseType& Pose;

		FRangedForSupport(const PoseType& InPose) : Pose(InPose) {};
		
		IterType begin() { return Pose.MakeBeginIter(); }
		IterType end() { return Pose.MakeEndIter(); }
	};

	template<typename PoseType, typename IterType>
	struct FRangedForReverseSupport
	{
		const PoseType& Pose;

		FRangedForReverseSupport(const PoseType& InPose) : Pose(InPose) {};

		IterType begin() { return Pose.MakeBeginIterReverse(); }
		IterType end() { return Pose.MakeEndIterReverse(); }
	};

protected:
	TArray<FTransform> Bones;
};


struct FMeshPose : public FBasePose<FMeshPoseBoneIndex>
{
public:
	typedef FMeshPoseBoneIndex BoneIndexType;

	const FBoneContainer& GetBoneContainer() const
	{
		checkSlow(BoneContainer && BoneContainer->IsValid());
		return *BoneContainer;
	}

	void SetBoneContainer(const FBoneContainer* InBoneContainer)
	{
		check(InBoneContainer && InBoneContainer->IsValid());
		BoneContainer = InBoneContainer;
		InitBones(BoneContainer->GetNumBones());
	}

	void InitFrom(FCompactPose& CompactPose);

	void MoveBonesTo(TArray<FTransform>& OutTransforms)
	{
		OutTransforms = MoveTemp(Bones);
		BoneContainer = NULL;
	}

	// Sets this pose to its ref pose
	ENGINE_API void ResetToRefPose();

	// Sets every bone transform to Identity
	void ResetToIdentity();

	// returns true if all bone rotations are normalized
	bool IsNormalized() const;

	// Returns true if any bone rotation contains NaN
	bool ContainsNaN() const;


	FORCEINLINE BoneIndexType GetParentBone(const BoneIndexType& BoneIndex)
	{
		return BoneIndexType(BoneContainer->GetParentBoneIndex(BoneIndex.GetInt()));
	}

protected:

	//Reference to our BoneContainer
	const FBoneContainer* BoneContainer;
};

struct FCompactPoseBoneIndexIterator
{
	int32 Index;

	FCompactPoseBoneIndexIterator(int32 InIndex) : Index(InIndex) {}

	FCompactPoseBoneIndexIterator& operator++() { ++Index; return (*this); }
	bool operator==(FCompactPoseBoneIndexIterator& Rhs) { return Index == Rhs.Index; }
	bool operator!=(FCompactPoseBoneIndexIterator& Rhs) { return Index != Rhs.Index; }
	FCompactPoseBoneIndex operator*() const { return FCompactPoseBoneIndex(Index); }
};

struct FCompactPoseBoneIndexReverseIterator
{
	int32 Index;

	FCompactPoseBoneIndexReverseIterator(int32 InIndex) : Index(InIndex) {}

	FCompactPoseBoneIndexReverseIterator& operator++() { --Index; return (*this); }
	bool operator==(FCompactPoseBoneIndexReverseIterator& Rhs) { return Index == Rhs.Index; }
	bool operator!=(FCompactPoseBoneIndexReverseIterator& Rhs) { return Index != Rhs.Index; }
	FCompactPoseBoneIndex operator*() const { return FCompactPoseBoneIndex(Index); }
};

struct FCompactPose : FBasePose<FCompactPoseBoneIndex>
{
public:
	typedef FCompactPoseBoneIndex BoneIndexType;

	//--------------------------------------------------------------------------
	//Bone Index Iteration
		typedef FBasePose<FCompactPoseBoneIndex>::FRangedForSupport<FCompactPose, FCompactPoseBoneIndexIterator> RangedForBoneIndexFwd;
		typedef FBasePose<FCompactPoseBoneIndex>::FRangedForReverseSupport<FCompactPose, FCompactPoseBoneIndexReverseIterator> RangedForBoneIndexBwd;

		FORCEINLINE RangedForBoneIndexFwd ForEachBoneIndex() const
		{
			return RangedForBoneIndexFwd(*this);
		}

		FORCEINLINE RangedForBoneIndexBwd ForEachBoneIndexReverse() const
		{
			return RangedForBoneIndexBwd(*this);
		}

		FORCEINLINE FCompactPoseBoneIndexIterator MakeBeginIter() const { return FCompactPoseBoneIndexIterator(0); }

		FORCEINLINE FCompactPoseBoneIndexIterator MakeEndIter() const { return FCompactPoseBoneIndexIterator(GetNumBones()); }

		FORCEINLINE FCompactPoseBoneIndexReverseIterator MakeBeginIterReverse() const { return FCompactPoseBoneIndexReverseIterator(GetNumBones() - 1); }

		FORCEINLINE FCompactPoseBoneIndexReverseIterator MakeEndIterReverse() const { return FCompactPoseBoneIndexReverseIterator(-1); }
	//--------------------------------------------------------------------------

	const FBoneContainer& GetBoneContainer() const 
	{
		checkSlow(BoneContainer && BoneContainer->IsValid());
		return *BoneContainer;
	}

	void SetBoneContainer(const FBoneContainer* InBoneContainer)
	{
		check(InBoneContainer && InBoneContainer->IsValid());
		BoneContainer = InBoneContainer;
		InitBones(BoneContainer->GetBoneIndicesArray().Num());
	}

	void InitFrom(const FCompactPose& SrcPose)
	{
		SetBoneContainer(SrcPose.BoneContainer);
		Bones = SrcPose.Bones;
	}

	// Moves transform data out of the supplied SrcPose (SrcPose will be left empty)
	void MoveBonesFrom(FCompactPose& SrcPose)
	{
		Bones = MoveTemp(SrcPose.Bones);
		BoneContainer = SrcPose.BoneContainer;
		SrcPose.BoneContainer = NULL;
	}

	//Moves transform data to supplied array (pose will be left empty)
	void MoveBonesTo(TArray<FTransform>& OutTransforms)
	{
		OutTransforms = MoveTemp(Bones);
		BoneContainer = NULL;
	}

	// Moves transform data out of the supplied InTransforms. InTransform will be left empty
	void MoveBonesFrom(TArray<FTransform>& InTransforms)
	{
		// if number doesn't match it is not compatible to start with
		if (InTransforms.Num() == Bones.Num())
		{
			Bones = MoveTemp(InTransforms);
		}
	}

	// Copy bone transform from SrcPose to this
	void CopyBonesFrom(FCompactPose& SrcPose)
	{
		if (this != &SrcPose)
		{
			Bones = SrcPose.Bones;
			BoneContainer = SrcPose.BoneContainer;
		}
	}

	// Sets this pose to its ref pose
	void ResetToRefPose()
	{
		ResetToRefPose(GetBoneContainer());
	}

	// Sets this pose to the supplied BoneContainers ref pose
	ENGINE_API void ResetToRefPose(const FBoneContainer& RequiredBones);

	// Sets every bone transform to Identity
	void ResetToIdentity();

	// returns true if all bone rotations are normalized
	bool IsNormalized() const;

	// Returns true if any bone rotation contains NaN
	bool ContainsNaN() const;

	// Normalizes all rotations in this pose
	void NormalizeRotations();

	bool IsValid() const
	{
		return (BoneContainer && BoneContainer->IsValid());
	}

	// Returns the bone index for the parent bone
	BoneIndexType GetParentBoneIndex(const BoneIndexType& BoneIndex) const
	{
		return GetBoneContainer().GetParentBoneIndex(BoneIndex);
	}

	// Returns the ref pose for the supplied bone
	const FTransform& GetRefPose(const BoneIndexType& BoneIndex) const
	{
		return GetBoneContainer().GetRefPoseTransform(BoneIndex);
	}

	// Populates this pose from the supplied animation and track data
	void PopulateFromAnimation(
		const UAnimSequence& Seq,
		const BoneTrackArray& RotationTracks,
		const BoneTrackArray& TranslationTracks,
		const BoneTrackArray& ScaleTracks,
		float Time)
	{
		AnimationFormat_GetAnimationPose(
			*((FTransformArray*)&Bones), //@TODO:@ANIMATION: Nasty hack
			RotationTracks,
			TranslationTracks,
			ScaleTracks,
			Seq,
			Time);
	}
protected:

	//Reference to our BoneContainer
	const FBoneContainer* BoneContainer;
};

template<class PoseType>
struct FCSPose
{
	// Set up our index type based on the type of pose we are manipulating
	typedef typename PoseType::BoneIndexType BoneIndexType;

	// Init Pose
	void InitPose(const FBoneContainer* InBoneContainer)
	{
		Pose.SetBoneContainer(InBoneContainer);
		Pose.ResetToRefPose();
		ComponentSpaceFlags.Empty(Pose.GetNumBones());
		ComponentSpaceFlags.AddZeroed(Pose.GetNumBones());
		ComponentSpaceFlags[0] = 1;
	}

	// Init Pose
	void InitPose(const PoseType& SrcPose)
	{
		Pose.InitFrom(SrcPose);
		ComponentSpaceFlags.Empty(Pose.GetNumBones());
		ComponentSpaceFlags.AddZeroed(Pose.GetNumBones());
		ComponentSpaceFlags[0] = 1;
	}
	const PoseType& GetPose() const { return Pose; }

	// Get transform for supplied bone in local space
	FTransform GetLocalSpaceTransform(BoneIndexType BoneIndex);

	// Get Transform for supplied bone in component space
	const FTransform& GetComponentSpaceTransform(BoneIndexType BoneIndex);

	// Set the transform for the supplied bone 
	void SetComponentSpaceTransform(BoneIndexType BoneIndex, const FTransform& NewTransform);

	// Calculate the component space transform for the supplied bone
	void CalculateComponentSpaceTransform(BoneIndexType BoneIndex);

	/**
	* Convert Bone to Local Space.
	*/
	void ConvertBoneToLocalSpace(BoneIndexType BoneIndex);

	/**
	* Set a bunch of Component Space Bone Transforms.
	* Do this safely by insuring that Parents are already in Component Space,
	* and any Component Space children are converted back to Local Space before hand.
	*/
	void SafeSetCSBoneTransforms(const TArray<struct FBoneTransform> & BoneTransforms);

	/**
	* Blends Component Space transforms to MeshPose in Local Space.
	* Used by SkelControls to apply their transforms.
	*
	* The tricky bit is that SkelControls deliver their transforms in Component Space,
	* But the blending is done in Local Space. Also we need to refresh any Children they have
	* that has been previously converted to Component Space.
	*/
	void LocalBlendCSBoneTransforms(const TArray<struct FBoneTransform>& BoneTransforms, float Alpha);

	// Convert any component space transforms back to local space
	void ConvertToLocalPoses(PoseType& OutPose);

protected:
	PoseType Pose;

	// Flags to track each bones current state (0 means local pose, 1 means component space pose)
	TCustomBoneIndexArray<uint8, BoneIndexType> ComponentSpaceFlags;
};

template<class PoseType>
FTransform FCSPose<PoseType>::GetLocalSpaceTransform(BoneIndexType BoneIndex)
{
	checkSlow(Pose.IsValid());
	check(Pose.IsValidIndex(BoneIndex));

	// if evaluated, calculate it
	if (ComponentSpaceFlags[BoneIndex])
	{
		const BoneIndexType ParentIndex = Pose.GetParentBoneIndex(BoneIndex);

		if (ParentIndex != INDEX_NONE)
		{
			const FTransform& ParentTransform = GetComponentSpaceTransform(ParentIndex);
			const FTransform& BoneTransform = Pose[BoneIndex];
			// calculate local space
			return BoneTransform.GetRelativeTransform(ParentTransform);
		}
	}

	return Pose[BoneIndex];
}

template<class PoseType>
const FTransform& FCSPose<PoseType>::GetComponentSpaceTransform(BoneIndexType BoneIndex)
{
	checkSlow(Pose.IsValid());
	check(Pose.IsValidIndex(BoneIndex));

	check(!Pose[BoneIndex].ContainsNaN());
	// if not evaluate, calculate it
	if (ComponentSpaceFlags[BoneIndex] == 0)
	{
		CalculateComponentSpaceTransform(BoneIndex);
	}
	check(!Pose[BoneIndex].ContainsNaN());
	return Pose[BoneIndex];
}

template<class PoseType>
void FCSPose<PoseType>::SetComponentSpaceTransform(BoneIndexType BoneIndex, const FTransform& NewTransform)
{
	checkSlow(Pose.IsValid());
	check(Pose.IsValidIndex(BoneIndex));

	// this one forcefully sets component space transform
	Pose[BoneIndex] = NewTransform;
	ComponentSpaceFlags[BoneIndex] = 1;
}

template<class PoseType>
void FCSPose<PoseType>::CalculateComponentSpaceTransform(BoneIndexType BoneIndex)
{
	checkSlow(Pose.IsValid());
	check(ComponentSpaceFlags[BoneIndex] == 0);

	// root is already verified, so root should not come here
	// check AllocateLocalPoses
	const BoneIndexType ParentIndex = Pose.GetParentBoneIndex(BoneIndex);

	// if Parent already has been calculated, use it
	if (ComponentSpaceFlags[ParentIndex] == 0)
	{
		// if Parent hasn't been calculated, also calculate parents
		CalculateComponentSpaceTransform(ParentIndex);
	}

	// current Bones(Index) should contain LocalPoses.
	FTransform& Bone = Pose[BoneIndex];
	FTransform& ParentBone = Pose[ParentIndex];
	check(!Pose[BoneIndex].ContainsNaN());
	check(!Pose[ParentIndex].ContainsNaN());
	FTransform Suspect = Pose[BoneIndex] * Pose[ParentIndex];
	check(!Suspect.ContainsNaN());
	Pose[BoneIndex] = Pose[BoneIndex] * Pose[ParentIndex];
	Pose[BoneIndex].NormalizeRotation();
	check(!Pose[BoneIndex].ContainsNaN());
	ComponentSpaceFlags[BoneIndex] = 1;
}

template<class PoseType>
void FCSPose<PoseType>::ConvertBoneToLocalSpace(BoneIndexType BoneIndex)
{
	checkSlow(Pose.IsValid());

	// If BoneTransform is in Component Space, then convert it.
	// Never convert Root to Local Space.
	if (!BoneIndex.IsRootBone() && ComponentSpaceFlags[BoneIndex] == 1)
	{
		const BoneIndexType ParentIndex = Pose.GetParentBoneIndex(BoneIndex);

		// Verify that our Parent is also in Component Space. That should always be the case.
		check(ComponentSpaceFlags[ParentIndex] == 1);

		// Convert to local space.
		Pose[BoneIndex].SetToRelativeTransform(Pose[ParentIndex]);
		ComponentSpaceFlags[BoneIndex] = 0;
	}
}

template<class PoseType>
void FCSPose<PoseType>::SafeSetCSBoneTransforms(const TArray<struct FBoneTransform> & BoneTransforms)
{
	checkSlow(Pose.IsValid());

	// Bone Mask to keep track of which bones have to be converted to local space.
	// This is basically BoneTransforms's children.
	TCustomBoneIndexArray<uint8, FCompactPoseBoneIndex> BoneMask;
	BoneMask.AddZeroed(Pose.GetNumBones());

	// First build our BoneMask
	for (const FBoneTransform& BoneTransform : BoneTransforms)
	{
		// Mark those bones in Mesh Pose as being required to be in Local Space.
		BoneMask[BoneTransform.BoneIndex] = 1;
	}


	//Flag children
	for (const FCompactPoseBoneIndex BoneIndex : Pose.ForEachBoneIndex())
	{
		const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
		// Propagate our BoneMask to children.
		if (ParentIndex != INDEX_NONE)
		{
			BoneMask[BoneIndex] |= BoneMask[ParentIndex];
		}
	}

	// now iterate from children to parent to calculate back to local space
	// Sadly this has to iterate from back because you'll need parent to be component space
	// if you do this from parent to child, you'll see parent turned to local before children applies it
	int32 BoneTransformIndex = BoneTransforms.Num() - 1;
	for (FCompactPoseBoneIndex BoneIndex : Pose.ForEachBoneIndexReverse())
	{
		// If this bone has to be converted to Local Space...
		if (BoneMask[BoneIndex] != 0)
		{
			// If this is one of the original BoneTransforms list bones
			// Then we don't actually want to convert this one to local space, since we're going to overwrite it.
			// So skip it.
			if (BoneTransformIndex >= 0 && BoneIndex == BoneTransforms[BoneTransformIndex].BoneIndex)
			{
				BoneTransformIndex--;
			}
			// If this is a children bone then we want it to be in local space!
			else
			{
				// .. If it is not currently in Local Space, then convert it.
				ConvertBoneToLocalSpace(BoneIndex);
			}
		}
	}

	// Finally copy our Component Space transforms
	for (const FBoneTransform& BoneTransform : BoneTransforms)
	{
		const FCompactPoseBoneIndex BoneIndex = BoneTransform.BoneIndex;

		// Make sure our BoneTransforms were in Component Space in the first place, before we overwrite them
		// Only check their parent to do minimal work needed.
		const FCompactPoseBoneIndex ParentBoneIndex = Pose.GetParentBoneIndex(BoneIndex);
		if (ParentBoneIndex != INDEX_NONE && ComponentSpaceFlags[ParentBoneIndex] == 0)
		{
			CalculateComponentSpaceTransform(ParentBoneIndex);
		}

		// Set new Component Space transform.
		SetComponentSpaceTransform(BoneIndex, BoneTransform.Transform);
	}
}

template<class PoseType>
void FCSPose<PoseType>::LocalBlendCSBoneTransforms(const TArray<struct FBoneTransform>& BoneTransforms, float Alpha)
{
	// if Alpha is small enough, skip
	if (Alpha < ZERO_ANIMWEIGHT_THRESH)
	{
		return;
	}

#if DO_CHECK
	if (BoneTransforms.Num() > 0)
	{
		FCompactPoseBoneIndex LastIndex(BoneTransforms[0].BoneIndex);
		// Make sure bones are sorted in "Parents before Children" order.
		for (int32 I = 1; I < BoneTransforms.Num(); ++I)
		{
			check(BoneTransforms[I].BoneIndex >= LastIndex);
			LastIndex = BoneTransforms[I].BoneIndex;
		}
	}
#endif

	// If we are not doing any blending, do a faster path.
	// Set transforms directly in Component Space. But still refresh children.
	if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH)
	{
		SafeSetCSBoneTransforms(BoneTransforms);
	}
	// Slower blending path.
	else
	{
		// Bone Mask to keep track of which bones have to be converted to local space.
		// This is basically BoneTransforms bones and their children.
		TCustomBoneIndexArray<uint8, BoneIndexType> BoneMask;
		BoneMask.AddZeroed(Pose.GetNumBones());

		TArray<struct FBoneTransform> LocalBoneTransforms;
		LocalBoneTransforms.SetNumUninitialized(BoneTransforms.Num());

		// First, convert BoneTransforms to local space for blending.
		for (int32 Index = 0; Index < BoneTransforms.Num(); Index++)
		{
			const BoneIndexType BoneIndex = BoneTransforms[Index].BoneIndex;
			const BoneIndexType ParentIndex = Pose.GetParentBoneIndex(BoneIndex);

			if (ParentIndex != INDEX_NONE)
			{
				// if BoneTransforms(modified by controllers) contains ParentIndex, it should use that as ParentTransform, not the one from input
				int32 LocalParentIndex = INDEX_NONE;
				for (int32 LocalIndex = 0; LocalIndex < BoneTransforms.Num(); ++LocalIndex)
				{
					if (ParentIndex == BoneTransforms[LocalIndex].BoneIndex)
					{
						LocalParentIndex = LocalIndex;
						break;
					}
				}

				// saves Parent Transform
				const bool bNoParent = LocalParentIndex == INDEX_NONE;
				const FTransform& ParentTransform = bNoParent ? GetComponentSpaceTransform(ParentIndex) : BoneTransforms[LocalParentIndex].Transform;

				LocalBoneTransforms[Index].Transform = BoneTransforms[Index].Transform.GetRelativeTransform(ParentTransform);
				LocalBoneTransforms[Index].BoneIndex = BoneIndex;

				// Mark those bones in Mesh Pose as being required to be in Local Space.
				BoneMask[BoneIndex] = 1;
			}
			else
			{
				// when root is entered as to modify, we don't need to adjust parent index, just clear it
				LocalBoneTransforms[Index].Transform = BoneTransforms[Index].Transform;
				LocalBoneTransforms[Index].BoneIndex = BoneIndex;

				BoneMask[BoneIndex] = 1;
			}
		}

		// Then, convert MeshPose Bones from BoneTransforms list, and their children, to local space if they are not already.
		for (const BoneIndexType BoneIndex : Pose.ForEachBoneIndex())
		{
			const BoneIndexType ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
			// Propagate our BoneMask to children.
			if (ParentIndex != INDEX_NONE)
			{
				BoneMask[BoneIndex] |= BoneMask[ParentIndex];
			}
		}

		for (const BoneIndexType BoneIndex : Pose.ForEachBoneIndexReverse())
		{
			if (!BoneIndex.IsRootBone())
			{
				// If this bone has to be converted to Local Space...
				if (BoneMask[BoneIndex] != 0)
				{
					// .. And is not currently in Local Space, then convert it.
					ConvertBoneToLocalSpace(BoneIndex);
				}
			}
		}

		// Lastly, do the blending in local space.
		for (int32 Index = 0; Index < LocalBoneTransforms.Num(); Index++)
		{
			const FCompactPoseBoneIndex BoneIndex = LocalBoneTransforms[Index].BoneIndex;
			// Make sure this transform is in local space, because we are writing a local space one to it.
			// If we are not in local space, this could mean trouble for our children.
			check((ComponentSpaceFlags[BoneIndex] == 0) || (BoneIndex == 0));

			// No need to normalize rotation since BlendWith() does it.
			Pose[BoneIndex].BlendWith(LocalBoneTransforms[Index].Transform, Alpha);
		}
	}
}

template<class PoseType>
void FCSPose<PoseType>::ConvertToLocalPoses(PoseType& OutPose)
{
	checkSlow(Pose.IsValid());
	OutPose = Pose;

	// now we need to convert back to local bases
	// only convert back that has been converted to mesh base
	// if it was local base, and if it hasn't been modified
	// that is still okay even if parent is changed, 
	// that doesn't mean this local has to change
	// go from child to parent since I need parent inverse to go back to local
	// root is same, so no need to do Index == 0
	for (const BoneIndexType BoneIndex : Pose.ForEachBoneIndexReverse())
	{
		if (!BoneIndex.IsRootBone())
		{
			// root is already verified, so root should not come here
			if (ComponentSpaceFlags[BoneIndex])
			{
				const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);

				OutPose[BoneIndex].SetToRelativeTransform(OutPose[ParentIndex]);
				OutPose[BoneIndex].NormalizeRotation();
			}
		}
	}
}