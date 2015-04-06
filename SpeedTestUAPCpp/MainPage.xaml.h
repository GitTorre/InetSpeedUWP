//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once
#include "MainPage.g.h"
#include "ppl.h"

using namespace InetSpeedUAP;
using namespace Concurrency;

namespace SpeedTestUAPCpp
{
	public ref class MainPage sealed
	{
	public:
		MainPage();
	private:
		void SpeedButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		ConnectionSpeed __speed;
	};
}
