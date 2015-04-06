//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace Blinks;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	blinkBackground(std::chrono::milliseconds(500), std::chrono::seconds(5));
	(void) e;	// Unused parameter
}

void MainPage::blinkBackground(std::chrono::milliseconds period, std::chrono::milliseconds duration)
{
	using namespace pplpp;
	using namespace concurrency;

	// For different blinking colors
	Array<Brush^>^ brushes = { ref new SolidColorBrush(Colors::Blue), ref new SolidColorBrush(Colors::Green),
		ref new SolidColorBrush(Colors::Yellow), ref new SolidColorBrush(Colors::Orange),
		ref new SolidColorBrush(Colors::Red), ref new SolidColorBrush(Colors::Purple) };

	// Used for cancel blinking process when the blinking duration reached.
	timed_cancellation_token_source blinkDelay;

	auto colorIndex = std::make_shared<size_t>(0);
	auto backupBrush = this->bg->Background;

	// An async loop used for keeping changing background colors.
	// Note that this cannot be replaced by a "for-loop" since the loop body contains async wait.
	create_iterative_task([=] {
		this->bg->Background = brushes->get(*colorIndex);

		if (++*colorIndex >= brushes->Length)
			*colorIndex = 0;

		// Use timer_task to do async wait. This wait will not block current thread (which is UI thread).
		// It controls blinking frequencies
		return create_timer_task(period, blinkDelay.get_token()).then([] {
			return true; // always return true to create a infinite loop. The loop will be stopped by the cancellation token.
		});
		// use concurrency::task_continuation_context::use_current() to enforce loop body running on current UI thread.
	}, concurrency::task_continuation_context::use_current(), blinkDelay.get_token()).then([= ](concurrency::task<void>) {
		// recover the background to the original color when the blink finlished.
		this->bg->Background = backupBrush;
	}, concurrency::task_continuation_context::use_current());

	// trigger cancellation after "duration" time
	blinkDelay.cancel(duration);
}
