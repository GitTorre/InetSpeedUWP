//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "ppl.h"

using namespace InetSpeedUAP;
using namespace Concurrency;

namespace UI
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
	private:
		void SpeedButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		ConnectionSpeed __speed;
	};
}
