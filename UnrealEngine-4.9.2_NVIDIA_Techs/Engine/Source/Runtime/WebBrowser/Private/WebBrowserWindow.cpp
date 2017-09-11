// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserWindow.h"
#include "WebBrowserByteResource.h"
#include "WebBrowserPopupFeatures.h"
#include "WebJSScripting.h"
#include "RHI.h"

#if WITH_CEF3

#if PLATFORM_MAC
// Needed for character code definitions
#include <Carbon/Carbon.h>
#include <AppKit/NSEvent.h>
#endif

#if PLATFORM_WINDOWS
#include "WindowsCursor.h"
typedef FWindowsCursor FPlatformCursor;
#elif PLATFORM_MAC
#include "MacCursor.h"
typedef FMacCursor FPlatformCursor;
#else
#endif

FWebBrowserWindow::FWebBrowserWindow(FIntPoint InViewportSize, FString InUrl, TOptional<FString> InContentsToLoad, bool InShowErrorMessage, bool InThumbMouseButtonNavigation, bool InUseTransparency)
	: DocumentState(EWebBrowserDocumentState::NoDocument)
	, CurrentUrl(InUrl)
	, ViewportSize(InViewportSize)
	, bIsClosing(false)
	, bIsInitialized(false)
	, ContentsToLoad(InContentsToLoad)
	, ShowErrorMessage(InShowErrorMessage)
	, bThumbMouseButtonNavigation(InThumbMouseButtonNavigation)
	, bUseTransparency(InUseTransparency)
	, Cursor(EMouseCursor::Default)
	, bIsHidden(false)
	, bTickedLastFrame(true)
	, PreviousKeyDownEvent()
	, PreviousKeyUpEvent()
	, PreviousCharacterEvent()
	, bIgnoreKeyDownEvent(false)
	, bIgnoreKeyUpEvent(false)
	, bIgnoreCharacterEvent(false)
	, bMainHasFocus(false)
	, bPopupHasFocus(false)
	, Scripting(new FWebJSScripting)
{
	UpdatableTextures[0] = nullptr;
	UpdatableTextures[1] = nullptr;
}

FWebBrowserWindow::~FWebBrowserWindow()
{
	CloseBrowser(true);
	if(FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid())
	{
		for (int I = 0; I < 1; ++I)
		{
			if (UpdatableTextures[I] != nullptr)
			{
				FSlateApplication::Get().GetRenderer()->ReleaseUpdatableTexture(UpdatableTextures[I]);
			}
		}
	}
	UpdatableTextures[0] = nullptr;
	UpdatableTextures[1] = nullptr;
}

void FWebBrowserWindow::LoadURL(FString NewURL)
{
	if (IsValid())
	{
		CefRefPtr<CefFrame> MainFrame = InternalCefBrowser->GetMainFrame();
		if (MainFrame.get() != nullptr)
		{
			ContentsToLoad = TOptional<FString>();
			CefString URL = *NewURL;
			MainFrame->LoadURL(URL);
		}
	}
}

void FWebBrowserWindow::LoadString(FString Contents, FString DummyURL)
{
	if (IsValid())
	{
		CefRefPtr<CefFrame> MainFrame = InternalCefBrowser->GetMainFrame();
		if (MainFrame.get() != nullptr)
		{
			ContentsToLoad = Contents;
			CefString URL = *DummyURL;
			MainFrame->LoadURL(URL);
		}
	}
}

void FWebBrowserWindow::SetViewportSize(FIntPoint WindowSize)
{
	// SetViewportSize is called from the browser viewport tick method, which means that since we are receiving ticks, we can mark the browser as visible.
	SetIsHidden(false);
	bTickedLastFrame=true;
	// Ignore sizes that can't be seen as it forces CEF to re-render whole image
	if (WindowSize.X > 0 && WindowSize.Y > 0 && ViewportSize != WindowSize)
	{
		ViewportSize = MoveTemp(WindowSize);
		
		if (IsValid())
		{
			InternalCefBrowser->GetHost()->WasResized();
		}
	}
}

FSlateShaderResource* FWebBrowserWindow::GetTexture(bool bIsPopup)
{
	if (UpdatableTextures[bIsPopup?1:0] != nullptr && IsInitialized())
	{
		return UpdatableTextures[bIsPopup?1:0]->GetSlateResource();
	}
	return nullptr;
}

bool FWebBrowserWindow::IsValid() const
{
	return InternalCefBrowser.get() != nullptr;
}

bool FWebBrowserWindow::IsInitialized() const
{
	return bIsInitialized;
}

bool FWebBrowserWindow::IsClosing() const
{
	return bIsClosing;
}

EWebBrowserDocumentState FWebBrowserWindow::GetDocumentLoadingState() const
{
	return DocumentState;
}

FString FWebBrowserWindow::GetTitle() const
{
	return Title;
}

FString FWebBrowserWindow::GetUrl() const
{
	if (InternalCefBrowser != nullptr)
	{
		CefRefPtr<CefFrame> MainFrame = InternalCefBrowser->GetMainFrame();

		if (MainFrame != nullptr)
		{
			return CurrentUrl;
		}
	}

	return FString();
}

void FWebBrowserWindow::PopulateCefKeyEvent(const FKeyEvent& InKeyEvent, CefKeyEvent& OutKeyEvent)
{
#if PLATFORM_MAC
	OutKeyEvent.native_key_code = InKeyEvent.GetKeyCode();

	FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::BackSpace)
	{
		OutKeyEvent.unmodified_character = kBackspaceCharCode;
	}
	else if (Key == EKeys::Tab)
	{
		OutKeyEvent.unmodified_character = kTabCharCode;
	}
	else if (Key == EKeys::Enter)
	{
		OutKeyEvent.unmodified_character = kReturnCharCode;
	}
	else if (Key == EKeys::Pause)
	{
		OutKeyEvent.unmodified_character = NSPauseFunctionKey;
	}
	else if (Key == EKeys::Escape)
	{
		OutKeyEvent.unmodified_character = kEscapeCharCode;
	}
	else if (Key == EKeys::PageUp)
	{
		OutKeyEvent.unmodified_character = NSPageUpFunctionKey;
	}
	else if (Key == EKeys::PageDown)
	{
		OutKeyEvent.unmodified_character = NSPageDownFunctionKey;
	}
	else if (Key == EKeys::End)
	{
		OutKeyEvent.unmodified_character = NSEndFunctionKey;
	}
	else if (Key == EKeys::Home)
	{
		OutKeyEvent.unmodified_character = NSHomeFunctionKey;
	}
	else if (Key == EKeys::Left)
	{
		OutKeyEvent.unmodified_character = NSLeftArrowFunctionKey;
	}
	else if (Key == EKeys::Up)
	{
		OutKeyEvent.unmodified_character = NSUpArrowFunctionKey;
	}
	else if (Key == EKeys::Right)
	{
		OutKeyEvent.unmodified_character = NSRightArrowFunctionKey;
	}
	else if (Key == EKeys::Down)
	{
		OutKeyEvent.unmodified_character = NSDownArrowFunctionKey;
	}
	else if (Key == EKeys::Insert)
	{
		OutKeyEvent.unmodified_character = NSInsertFunctionKey;
	}
	else if (Key == EKeys::Delete)
	{
		OutKeyEvent.unmodified_character = kDeleteCharCode;
	}
	else if (Key == EKeys::F1)
	{
		OutKeyEvent.unmodified_character = NSF1FunctionKey;
	}
	else if (Key == EKeys::F2)
	{
		OutKeyEvent.unmodified_character = NSF2FunctionKey;
	}
	else if (Key == EKeys::F3)
	{
		OutKeyEvent.unmodified_character = NSF3FunctionKey;
	}
	else if (Key == EKeys::F4)
	{
		OutKeyEvent.unmodified_character = NSF4FunctionKey;
	}
	else if (Key == EKeys::F5)
	{
		OutKeyEvent.unmodified_character = NSF5FunctionKey;
	}
	else if (Key == EKeys::F6)
	{
		OutKeyEvent.unmodified_character = NSF6FunctionKey;
	}
	else if (Key == EKeys::F7)
	{
		OutKeyEvent.unmodified_character = NSF7FunctionKey;
	}
	else if (Key == EKeys::F8)
	{
		OutKeyEvent.unmodified_character = NSF8FunctionKey;
	}
	else if (Key == EKeys::F9)
	{
		OutKeyEvent.unmodified_character = NSF9FunctionKey;
	}
	else if (Key == EKeys::F10)
	{
		OutKeyEvent.unmodified_character = NSF10FunctionKey;
	}
	else if (Key == EKeys::F11)
	{
		OutKeyEvent.unmodified_character = NSF11FunctionKey;
	}
	else if (Key == EKeys::F12)
	{
		OutKeyEvent.unmodified_character = NSF12FunctionKey;
	}
	else if (Key == EKeys::CapsLock)
	{
		OutKeyEvent.unmodified_character = 0;
		OutKeyEvent.native_key_code = kVK_CapsLock;
	}
	else if (Key.IsModifierKey())
	{
		// Setting both unmodified_character and character to 0 tells CEF that it needs to generate a NSFlagsChanged event instead of NSKeyDown/Up
		OutKeyEvent.unmodified_character = 0;

		// CEF expects modifier key codes as one of the Carbon kVK_* key codes.
		if (Key == EKeys::LeftCommand)
		{
			OutKeyEvent.native_key_code = kVK_Command;
		}
		else if (Key == EKeys::LeftShift)
		{
			OutKeyEvent.native_key_code = kVK_Shift;
		}
		else if (Key == EKeys::LeftAlt)
		{
			OutKeyEvent.native_key_code = kVK_Option;
		}
		else if (Key == EKeys::LeftControl)
		{
			OutKeyEvent.native_key_code = kVK_Control;
		}
		else if (Key == EKeys::RightCommand)
		{
			// There isn't a separate code for the right hand command key defined, but CEF seems to use the unused value before the left command keycode
			OutKeyEvent.native_key_code = kVK_Command-1;
		}
		else if (Key == EKeys::RightShift)
		{
			OutKeyEvent.native_key_code = kVK_RightShift;
		}
		else if (Key == EKeys::RightAlt)
		{
			OutKeyEvent.native_key_code = kVK_RightOption;
		}
		else if (Key == EKeys::RightControl)
		{
			OutKeyEvent.native_key_code = kVK_RightControl;
		}
	}
	else
	{
		OutKeyEvent.unmodified_character = InKeyEvent.GetCharacter();
	}
	OutKeyEvent.character = OutKeyEvent.unmodified_character;

#else
	OutKeyEvent.windows_key_code = InKeyEvent.GetKeyCode();
		// TODO: Figure out whether this is a system key if we come across problems
		/*KeyEvent.is_system_key = message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;*/
#endif

	OutKeyEvent.modifiers = GetCefKeyboardModifiers(InKeyEvent);
}

bool FWebBrowserWindow::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	if (IsValid() && !bIgnoreKeyDownEvent)
	{
		PreviousKeyDownEvent = InKeyEvent;
		CefKeyEvent KeyEvent;
		PopulateCefKeyEvent(InKeyEvent, KeyEvent);
		KeyEvent.type = KEYEVENT_RAWKEYDOWN;
		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
		return true;
	}
	return false;
}

bool FWebBrowserWindow::OnKeyUp(const FKeyEvent& InKeyEvent)
{
	if (IsValid() && !bIgnoreKeyUpEvent)
	{
		PreviousKeyUpEvent = InKeyEvent;
		CefKeyEvent KeyEvent;
		PopulateCefKeyEvent(InKeyEvent, KeyEvent);
		KeyEvent.type = KEYEVENT_KEYUP;
		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
		return true;
	}
	return false;
}

bool FWebBrowserWindow::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	if (IsValid() && !bIgnoreCharacterEvent)
	{
		PreviousCharacterEvent = InCharacterEvent;
		CefKeyEvent KeyEvent;
#if PLATFORM_MAC
		KeyEvent.character = InCharacterEvent.GetCharacter();
#else
		KeyEvent.windows_key_code = InCharacterEvent.GetCharacter();
#endif
		// TODO: Figure out whether this is a system key if we come across problems
		/*KeyEvent.is_system_key = message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;*/

		KeyEvent.type = KEYEVENT_CHAR;
		KeyEvent.modifiers = GetCefInputModifiers(InCharacterEvent);

		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
		return true;
	}
	return false;
}

/* This is an ugly hack to inject unhandled key events back into Slate.
   During processing of the initial keyboard event, we don't know whether it is handled by the Web browser or not.
   Not until after CEF calls OnKeyEvent in our CefKeyboardHandler implementation, which is after our own keyboard event handler
   has returned.
   The solution is to save a copy of the event and re-inject it into Slate while ensuring that we'll ignore it and bubble it up
   the widget hierarchy this time around. */
bool FWebBrowserWindow::OnUnhandledKeyEvent(const CefKeyEvent& CefEvent)
{
	bool bWasHandled = false;
	if (IsValid())
	{
		switch (CefEvent.type) {
			case KEYEVENT_RAWKEYDOWN:
			case KEYEVENT_KEYDOWN:
				if (PreviousKeyDownEvent.IsSet())
				{
					bIgnoreKeyDownEvent = true;
					bWasHandled = FSlateApplication::Get().ProcessKeyDownEvent(PreviousKeyDownEvent.GetValue());
					PreviousKeyDownEvent.Reset();
					bIgnoreKeyDownEvent = false;
				}
				break;
			case KEYEVENT_KEYUP:
				if (PreviousKeyUpEvent.IsSet())
				{
					bIgnoreKeyUpEvent = true;
					bWasHandled = FSlateApplication::Get().ProcessKeyUpEvent(PreviousKeyUpEvent.GetValue());
					PreviousKeyUpEvent.Reset();
					bIgnoreKeyUpEvent = false;
				}
				break;
			case KEYEVENT_CHAR:
				if (PreviousCharacterEvent.IsSet())
				{
					bIgnoreCharacterEvent = true;
					bWasHandled = FSlateApplication::Get().ProcessKeyCharEvent(PreviousCharacterEvent.GetValue());
					PreviousCharacterEvent.Reset();
					bIgnoreCharacterEvent = false;
				}
				break;
		  default:
			break;
	}
	}
	return bWasHandled;
}


bool FWebBrowserWindow::SupportsNewWindows()
{
	return OnCreateWindow().IsBound() ? true : false;
}

bool FWebBrowserWindow::RequestCreateWindow( const TSharedRef<IWebBrowserWindow>& NewBrowserWindow, const TSharedPtr<IWebBrowserPopupFeatures>& BrowserPopupFeatures )
{
	if(OnCreateWindow().IsBound())
	{
		return OnCreateWindow().Execute(TWeakPtr<IWebBrowserWindow>(NewBrowserWindow), TWeakPtr<IWebBrowserPopupFeatures>(BrowserPopupFeatures));
	}
	return false;
}

FReply FWebBrowserWindow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	FReply Reply = FReply::Unhandled();
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		// CEF only supports left, right, and middle mouse buttons
		bool bIsCefSupportedButton = (Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton);

		if(bIsCefSupportedButton)
		{
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event = GetCefMouseEvent(MyGeometry, MouseEvent, bIsPopup);
		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, false,1);
			Reply = FReply::Handled();
		}
	}
	return Reply;
}

FReply FWebBrowserWindow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	FReply Reply = FReply::Unhandled();
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		// CEF only supports left, right, and middle mouse buttons
		bool bIsCefSupportedButton = (Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton);

		if(bIsCefSupportedButton)
		{
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event = GetCefMouseEvent(MyGeometry, MouseEvent, bIsPopup);
		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, true, 1);
			Reply = FReply::Handled();
		}
		else if(Button == EKeys::ThumbMouseButton && bThumbMouseButtonNavigation)
		{
			if(CanGoBack())
			{
				GoBack();
				Reply = FReply::Handled();
			}
			
		}
		else if(Button == EKeys::ThumbMouseButton2 && bThumbMouseButtonNavigation)
		{
			if(CanGoForward())
			{
				GoForward();
				Reply = FReply::Handled();
			}
			
		}
	}
	return Reply;
}

FReply FWebBrowserWindow::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	FReply Reply = FReply::Unhandled();
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		// CEF only supports left, right, and middle mouse buttons
		bool bIsCefSupportedButton = (Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton);

		if(bIsCefSupportedButton)
		{
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event = GetCefMouseEvent(MyGeometry, MouseEvent, bIsPopup);
		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, false, 2);
			Reply = FReply::Handled();
	}
	}
	return Reply;
}

FReply FWebBrowserWindow::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	FReply Reply = FReply::Unhandled();
	if (IsValid())
	{
		CefMouseEvent Event = GetCefMouseEvent(MyGeometry, MouseEvent, bIsPopup);
		InternalCefBrowser->GetHost()->SendMouseMoveEvent(Event, false);
		Reply = FReply::Handled();
	}
	return Reply;
}

FReply FWebBrowserWindow::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	FReply Reply = FReply::Unhandled();
	if(IsValid())
	{
		// The original delta is reduced so this should bring it back to what CEF expects
		const float SpinFactor = 50.0f;
		const float TrueDelta = MouseEvent.GetWheelDelta() * SpinFactor;
		CefMouseEvent Event = GetCefMouseEvent(MyGeometry, MouseEvent, bIsPopup);
		InternalCefBrowser->GetHost()->SendMouseWheelEvent(Event,
															MouseEvent.IsShiftDown() ? TrueDelta : 0,
															!MouseEvent.IsShiftDown() ? TrueDelta : 0);
		Reply = FReply::Handled();
	}
	return Reply;
}

void FWebBrowserWindow::OnFocus(bool SetFocus, bool bIsPopup)
{
	if (bIsPopup)
	{
		bPopupHasFocus = SetFocus;
	}
	else
	{
		bMainHasFocus = SetFocus;
	}

	// Only notify focus if there is no popup menu with focus, as SendFocusEvent will dismiss any popup menus.
	if (IsValid() && !bPopupHasFocus)
	{
		InternalCefBrowser->GetHost()->SendFocusEvent(bMainHasFocus);
	}
}

void FWebBrowserWindow::OnCaptureLost()
{
	if (IsValid())
	{
		InternalCefBrowser->GetHost()->SendCaptureLostEvent();
	}
}

bool FWebBrowserWindow::CanGoBack() const
{
	if (IsValid())
	{
		return InternalCefBrowser->CanGoBack();
	}
	return false;
}

void FWebBrowserWindow::GoBack()
{
	if (IsValid())
	{
		InternalCefBrowser->GoBack();
	}
}

bool FWebBrowserWindow::CanGoForward() const
{
	if (IsValid())
	{
		return InternalCefBrowser->CanGoForward();
	}
	return false;
}

void FWebBrowserWindow::GoForward()
{
	if (IsValid())
	{
		InternalCefBrowser->GoForward();
	}
}

bool FWebBrowserWindow::IsLoading() const
{
	if (IsValid())
	{
		return InternalCefBrowser->IsLoading();
	}
	return false;
}

void FWebBrowserWindow::Reload()
{
	if (IsValid())
	{
		InternalCefBrowser->Reload();
	}
}

void FWebBrowserWindow::StopLoad()
{
	if (IsValid())
	{
		InternalCefBrowser->StopLoad();
	}
}

void FWebBrowserWindow::ExecuteJavascript(const FString& Script)
{
	if (IsValid())
	{
		CefRefPtr<CefFrame> frame = InternalCefBrowser->GetMainFrame();
		frame->ExecuteJavaScript(TCHAR_TO_ANSI(*Script), frame->GetURL(), 0);
	}
}

void FWebBrowserWindow::SetHandler(CefRefPtr<FWebBrowserHandler> InHandler)
{
	if (InHandler.get())
	{
		Handler = InHandler;
		Handler->SetBrowserWindow(SharedThis(this));
		Handler->SetShowErrorMessage(ShowErrorMessage);
	}
}

void FWebBrowserWindow::CloseBrowser(bool bForce)
{
	if (IsValid())
	{
		InternalCefBrowser->GetHost()->CloseBrowser(bForce);
	}
}

void FWebBrowserWindow::BindCefBrowser(CefRefPtr<CefBrowser> Browser)
{
	check(Browser.get() == nullptr || InternalCefBrowser.get() == nullptr || InternalCefBrowser->IsSame(Browser));
	Scripting->BindCefBrowser(Browser); // The scripting interface needs the browser too
	InternalCefBrowser = Browser;
}

CefRefPtr<CefBrowser> FWebBrowserWindow::GetCefBrowser()
{
	return InternalCefBrowser;
}

void FWebBrowserWindow::SetTitle(const CefString& InTitle)
{
	Title = InTitle.ToWString().c_str();
	TitleChangedEvent.Broadcast(Title);
}

void FWebBrowserWindow::SetUrl(const CefString& Url)
{
	CurrentUrl = Url.ToWString().c_str();
	OnUrlChanged().Broadcast(CurrentUrl);
}

void FWebBrowserWindow::SetToolTip(const CefString& CefToolTip)
{
	FString NewToolTipText = CefToolTip.ToWString().c_str();
	if (ToolTipText != NewToolTipText)
	{
		ToolTipText = NewToolTipText;
		OnToolTip().Broadcast(ToolTipText);
	}
}


bool FWebBrowserWindow::GetViewRect(CefRect& Rect)
{
	if (ViewportSize == FIntPoint::ZeroValue)
	{
		return false;
	}
	
	Rect.x = 0;
	Rect.y = 0;
	Rect.width = ViewportSize.X;
	Rect.height = ViewportSize.Y;

	return true;
}

void FWebBrowserWindow::NotifyDocumentError()
{
	DocumentState = EWebBrowserDocumentState::Error;
	DocumentStateChangedEvent.Broadcast(DocumentState);
}

void FWebBrowserWindow::NotifyDocumentLoadingStateChange(bool IsLoading)
{
	if (! IsLoading)
	{
		bIsInitialized = true;
	}
	
	EWebBrowserDocumentState NewState = IsLoading
		? EWebBrowserDocumentState::Loading
		: EWebBrowserDocumentState::Completed;

	if (DocumentState != EWebBrowserDocumentState::Error)
	{
		DocumentState = NewState;
	}

	DocumentStateChangedEvent.Broadcast(NewState);
}

void FWebBrowserWindow::OnPaint(CefRenderHandler::PaintElementType Type, const CefRenderHandler::RectList& DirtyRects, const void* Buffer, int Width, int Height)
{

	if (UpdatableTextures[Type] == nullptr && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid())
	{
		UpdatableTextures[Type] = FSlateApplication::Get().GetRenderer()->CreateUpdatableTexture(Width,Height);
	}

	if (UpdatableTextures[Type] != nullptr)
	{
		// Note that with more recent versions of CEF, the DirtyRects will always contain a single element, as it merges all dirty areas into a single rectangle before calling OnPaint
		// In case that should change in the future, we'll simply update the entire area if DirtyRects is not a single element.
		FIntRect Dirty = (DirtyRects.size() == 1)?FIntRect(DirtyRects[0].x, DirtyRects[0].y, DirtyRects[0].x + DirtyRects[0].width, DirtyRects[0].y + DirtyRects[0].height):FIntRect();
		UpdatableTextures[Type]->UpdateTextureThreadSafeRaw(Width, Height, Buffer, Dirty);
	}

	bIsInitialized = true;
	NeedsRedrawEvent.Broadcast();
}

void FWebBrowserWindow::OnCursorChange(CefCursorHandle CefCursor, CefRenderHandler::CursorType Type, const CefCursorInfo& CustomCursorInfo)
{
	switch (Type) {
		// Map the basic 3 cursor types directly to Slate types on all platforms
		case CT_NONE:
			Cursor = EMouseCursor::None;
			break;
		case CT_POINTER:
			Cursor = EMouseCursor::Default;
			break;
		case CT_IBEAM:
			Cursor = EMouseCursor::TextEditBeam;
			break;
		#if PLATFORM_WINDOWS || PLATFORM_MAC
		// Platform specific support for native cursor types
		default:
			{
				FPlatformCursor* PlatformCursor = (FPlatformCursor*)FSlateApplication::Get().GetPlatformCursor().Get();
				PlatformCursor->SetCustomShape(CefCursor);
				Cursor = EMouseCursor::Custom;
			}
			break;
		#else
		// Map to closest Slate equivalent on platforms where native cursors are not available.
		case CT_VERTICALTEXT:
			Cursor = EMouseCursor::TextEditBeam;
			break;
		case CT_EASTRESIZE:
		case CT_WESTRESIZE:
		case CT_EASTWESTRESIZE:
		case CT_COLUMNRESIZE:
			Cursor = EMouseCursor::ResizeLeftRight;
			break;
		case CT_NORTHRESIZE:
		case CT_SOUTHRESIZE:
		case CT_NORTHSOUTHRESIZE:
		case CT_ROWRESIZE:
			Cursor = EMouseCursor::ResizeUpDown;
			break;
		case CT_NORTHWESTRESIZE:
		case CT_SOUTHEASTRESIZE:
		case CT_NORTHWESTSOUTHEASTRESIZE:
			Cursor = EMouseCursor::ResizeSouthEast;
			break;
		case CT_NORTHEASTRESIZE:
		case CT_SOUTHWESTRESIZE:
		case CT_NORTHEASTSOUTHWESTRESIZE:
			Cursor = EMouseCursor::ResizeSouthWest;
			break;
		case CT_MOVE:
		case CT_MIDDLEPANNING:
		case CT_EASTPANNING:
		case CT_NORTHPANNING:
		case CT_NORTHEASTPANNING:
		case CT_NORTHWESTPANNING:
		case CT_SOUTHPANNING:
		case CT_SOUTHEASTPANNING:
		case CT_SOUTHWESTPANNING:
		case CT_WESTPANNING:
			Cursor = EMouseCursor::CardinalCross;
			break;
		case CT_CROSS:
			Cursor = EMouseCursor::Crosshairs;
			break;
		case CT_HAND:
			Cursor = EMouseCursor::Hand;
			break;
		case CT_GRAB:
			Cursor = EMouseCursor::GrabHand;
			break;
		case CT_GRABBING:
			Cursor = EMouseCursor::GrabHandClosed;
			break;
		case CT_NOTALLOWED:
		case CT_NODROP:
			Cursor = EMouseCursor::SlashedCircle;
			break;
		default:
			Cursor = EMouseCursor::Default;
			break;
		#endif
	}
	// Tell Slate to update the cursor now
	FSlateApplication::Get().QueryCursor();
}


bool FWebBrowserWindow::OnBeforeBrowse( CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, CefRefPtr<CefRequest> Request, bool bIsRedirect )
{
	if (InternalCefBrowser != nullptr && InternalCefBrowser->IsSame(Browser))
	{
		CefRefPtr<CefFrame> MainFrame = InternalCefBrowser->GetMainFrame();
		if (MainFrame.get() != nullptr)
		{
			if(OnBeforeBrowse().IsBound())
			{
				FString Url = Request->GetURL().ToWString().c_str();
				return OnBeforeBrowse().Execute(Url, bIsRedirect);
			}
		}
	}
	return false;
}

CefRefPtr<CefResourceHandler> FWebBrowserWindow::GetResourceHandler(CefRefPtr< CefFrame > Frame, CefRefPtr< CefRequest > Request )
{
	if (ContentsToLoad.IsSet())
	{
		FTCHARToUTF8 Convert(*ContentsToLoad.GetValue());
		CefRefPtr<CefResourceHandler> Resource = new FWebBrowserByteResource(Convert.Get(), Convert.Length());
		ContentsToLoad = TOptional<FString>();
		return Resource;
	}
	if (OnLoadUrl().IsBound())
	{
		FString Method = Request->GetMethod().ToWString().c_str();
		FString Url = Request->GetURL().ToWString().c_str();
		FString Response;
		if ( OnLoadUrl().Execute(Method, Url, Response))
	{
			FTCHARToUTF8 Convert(*Response);
			CefRefPtr<CefResourceHandler> Resource = new FWebBrowserByteResource(Convert.Get(), Convert.Length());
			return Resource;
		}
	}
	return NULL;
}


int32 FWebBrowserWindow::GetCefKeyboardModifiers(const FKeyEvent& KeyEvent)
{
	int32 Modifiers = GetCefInputModifiers(KeyEvent);

	const FKey Key = KeyEvent.GetKey();
	if (Key == EKeys::LeftAlt ||
		Key == EKeys::LeftCommand ||
		Key == EKeys::LeftControl ||
		Key == EKeys::LeftShift)
	{
		Modifiers |= EVENTFLAG_IS_LEFT;
	}
	if (Key == EKeys::RightAlt ||
		Key == EKeys::RightCommand ||
		Key == EKeys::RightControl ||
		Key == EKeys::RightShift)
	{
		Modifiers |= EVENTFLAG_IS_RIGHT;
	}
	if (Key == EKeys::NumPadZero ||
		Key == EKeys::NumPadOne ||
		Key == EKeys::NumPadTwo ||
		Key == EKeys::NumPadThree ||
		Key == EKeys::NumPadFour ||
		Key == EKeys::NumPadFive ||
		Key == EKeys::NumPadSix ||
		Key == EKeys::NumPadSeven ||
		Key == EKeys::NumPadEight ||
		Key == EKeys::NumPadNine)
	{
		Modifiers |= EVENTFLAG_IS_KEY_PAD;
	}

	return Modifiers;
}

int32 FWebBrowserWindow::GetCefMouseModifiers(const FPointerEvent& InMouseEvent)
{
	int32 Modifiers = GetCefInputModifiers(InMouseEvent);

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		Modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
	}
	if (InMouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
	{
		Modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
	}
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		Modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
	}

	return Modifiers;
}

CefMouseEvent FWebBrowserWindow::GetCefMouseEvent(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	CefMouseEvent Event;
	FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	Event.x = LocalPos.X;
	Event.y = LocalPos.Y;
	if (bIsPopup)
	{
		Event.x += PopupRect.Min.X;
		Event.y += PopupRect.Min.Y;
	}
	Event.modifiers = GetCefMouseModifiers(MouseEvent);
	return Event;
}

int32 FWebBrowserWindow::GetCefInputModifiers(const FInputEvent& InputEvent)
{
	int32 Modifiers = 0;

	if (InputEvent.IsShiftDown())
	{
		Modifiers |= EVENTFLAG_SHIFT_DOWN;
	}
	if (InputEvent.IsControlDown())
	{
#if PLATFORM_MAC
		// Slate swaps the flags for Command and Control on OSX, so we need to swap them back for CEF
		Modifiers |= EVENTFLAG_COMMAND_DOWN;
#else
		Modifiers |= EVENTFLAG_CONTROL_DOWN;
#endif
	}
	if (InputEvent.IsAltDown())
	{
		Modifiers |= EVENTFLAG_ALT_DOWN;
	}
	if (InputEvent.IsCommandDown())
	{
#if PLATFORM_MAC
		// Slate swaps the flags for Command and Control on OSX, so we need to swap them back for CEF
		Modifiers |= EVENTFLAG_CONTROL_DOWN;
#else
		Modifiers |= EVENTFLAG_COMMAND_DOWN;
#endif
	}
	if (InputEvent.AreCapsLocked())
	{
		Modifiers |= EVENTFLAG_CAPS_LOCK_ON;
	}
	// TODO: Add function for this if necessary
	/*if (InputEvent.AreNumsLocked())
	{
		Modifiers |= EVENTFLAG_NUM_LOCK_ON;
	}*/

	return Modifiers;
}

void FWebBrowserWindow::CheckTickActivity()
{
	// Early out if we're currently hidden, not initialized or currently loading.
	if (bIsHidden || !IsValid() || IsLoading() || ViewportSize == FIntPoint::ZeroValue)
	{
		return;
	}

	// We clear the bTickedLastFrame flag here and set it on every Slate tick.
	// If it's still clear when we come back it means we're not getting ticks from slate.
	// Note: The BrowserSingleton object will not invoke this method if Slate itself is sleeping.
	// Therefore we can safely assume the widget is hidden in that case.
	if (!bTickedLastFrame)
	{
		SetIsHidden(true);
	}
	bTickedLastFrame = false;
}

void FWebBrowserWindow::SetIsHidden(bool bValue)
{
	if( bIsHidden == bValue )
	{
		return;
	}
	bIsHidden = bValue;
	if ( IsValid() )
	{
		InternalCefBrowser->GetHost()->WasHidden(bIsHidden);
	}
}

CefRefPtr<CefDictionaryValue> FWebBrowserWindow::GetProcessInfo()
{
	CefRefPtr<CefDictionaryValue> Retval = nullptr;
	if (IsValid())
	{
		Retval = CefDictionaryValue::Create();
		Retval->SetInt("browser", InternalCefBrowser->GetIdentifier());
		Retval->SetDictionary("bindings", Scripting->GetPermanentBindings());
	}
	return Retval;
}

bool FWebBrowserWindow::OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser, CefProcessId SourceProcess, CefRefPtr<CefProcessMessage> Message)
{
	return Scripting->OnProcessMessageReceived(Browser, SourceProcess, Message);
}

void FWebBrowserWindow::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	Scripting->BindUObject(Name, Object, bIsPermanent);
}

void FWebBrowserWindow::UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	Scripting->UnbindUObject(Name, Object, bIsPermanent);
}

bool FWebBrowserWindow::OnCefBeforePopup(const CefString& Target_Url, const CefString& Target_Frame_Name)
{
	if (OnBeforePopup().IsBound())
	{
		FString URL = Target_Url.ToWString().c_str();
		FString FrameName = Target_Frame_Name.ToWString().c_str();
		return OnBeforePopup().Execute(URL, FrameName);
	}

	return false;
}

void FWebBrowserWindow::OnBrowserClosing()
{
	bIsClosing = true;
}

void FWebBrowserWindow::OnBrowserClosed()
{
	if(OnCloseWindow().IsBound())
	{
		OnCloseWindow().Execute(TWeakPtr<IWebBrowserWindow>(SharedThis(this)));
	}

	Scripting->UnbindCefBrowser();
	InternalCefBrowser = nullptr;
	Handler = nullptr;
}

void FWebBrowserWindow::ShowPopup(CefRect CefPopupSize)
{
	PopupRect = FIntRect(CefPopupSize.x, CefPopupSize.y, CefPopupSize.x+CefPopupSize.width, CefPopupSize.y+CefPopupSize.height);
	bPopupHasFocus = true;
	OnShowPopup().Broadcast(PopupRect);
}

void FWebBrowserWindow::HidePopup()
{
	bPopupHasFocus = false;
	OnDismissPopup().Broadcast();
}


#endif
