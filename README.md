Internet Connection Speed API for Windows 10 UWP, Version 1.0

This is a convenient Windows multi-language API for making Internet connection state decisions in real time, using the speed result to decide if/when to run your network-intensive code. 

API

InetSpeedUWP

class InternetConnectionState 

object that contains the static members you use to determine Internet connection state (connection status and speed/latency/delay) 

 Properties 

 static bool Connected 
 
 Returns true if the current Internet connection for the device is active, else false. 

 Methods 

 static IAsyncOperation<ConnectionSpeed> GetInternetConnectionSpeed(); 
 
 Asynchronous method that will return a ConnectionSpeed (see below). 

static IAsyncOperation<ConnectionSpeed> GetInternetConnectionSpeedWithHostName(HostName hostName); 

Asynchronous method that will perform the speed/latency test on a supplied host target and returns a ConnectionSpeed. This is very useful to ensure the Internet resource you’re trying to reach is available at the speed level you require (generally, these would be High and Average…). 

enum class ConnectionSpeed 

Speed test results are returned as an enum value (For JavaScript consumers, you’ll need to build your own object mapping. See the JavaScript example). 

High: Device is currently attached to a high-speed, low-latency Internet connection. 

Average: Device is currently attached to an average speed/latency Internet connection (LTE, 3G, etc…). 

Low: Device is currently attached to a low-speed, high-latency Internet connection. 

Unknown: The current Internet connection speed can't be determined. Proceed with caution. This could mean that there is very high network latency, a problem with an upstream service, etc... 

Example (C# consumer): 

This example tests for a highspeed network based on a provided HostName (this is the best way to use this API given you really want to know the status of the Internet connection as it pertains to where you need to put/grab data over the network in real time...). Note you should always test for Unknown and then react accordingly (don't proceed with network work. Unknown means you are connected to the Internet, but you can't do network work with acceptable latency.) 

High and Average are the two results you should test for before doing network work that involves either downloading or uploading data.

            using InetSpeedUWP;

            ...

             if (InternetConnectionState.Connected) 
             { 
                 var speed = 
                     await InternetConnectionState.GetInternetConnectionSpeedWithHostName(new HostName("targethost.com")); 
             
                 if (speed == ConnectionSpeed.High) 
                 { 
                     \\Current network speed is High, low latency 
                 } 
                 else if (speed == ConnectionSpeed.Average) 
                 { 
                     \\Current network speed is Average, average latency 
                 } 
                 else if (speed == ConnectionSpeed.Low) 
                 { 
                     \\Current network speed is Low, high latency 
                 } 
                 else 
                 { 
                     \\Current Network Speed is Unknown - use at your own risk... 
                 } 
             } 
             else 
             { 
                 ResultsBox.Text = "Not Connected to the Internet!"; 
             } 
         } 

Example (JavaScript consumer): 

The JavaScript WinRT projection doesn't support C++ enum class named values. Instead, the enum integer values are passed along... 

The following makes the code easier to read -> apply named values (JS objects) to numbers (the speed result integer) received from the native API...

   ```JS
   var ConnectionSpeed = { High: 0, Average: 1, Low: 2, Unknown: 3 }; 

   function getConnectionSpeed() { 
       if (InetSpeedUWP.InternetConnectionState.connected) { 
               InetSpeedUWP.InternetConnectionState.getInternetConnectionSpeedWithHostName( 
                 new Windows.Networking.HostName("mytargethost.com")) 
                 .then(function (speed) { 
                 if (speed === ConnectionSpeed.Unknown) return; 
                 if (speed === ConnectionSpeed.High) { 
                           //Highspeed, low latency Internet connection detected... 
                 } 
                 else if  (speed === ConnectionSpeed.Average){ 
                           //Average speed, average latency Internet connection detected... 
                 } 
                 else if (speed ===  ConnectionSpeed.Low) { 
                           //High latency, low speed Internet connection detected... 
                 } 
           }); 
       } 
       else { 
                 //Not connected... 
 } 
```
Example (C++/CX consumer): 

	if (InternetConnectionState::Connected)
	{
		auto connectionSpeedWithHost = InternetConnectionState::GetInternetConnectionSpeedWithHostName(ref new Windows::Networking::HostName("pinterest.com"));
		auto _speed = create_task(connectionSpeedWithHost);
		_speed.then([this](ConnectionSpeed speed)
		{
			if (speed == ConnectionSpeed::Unknown) return;
			TextBoxResults->Text += speed.ToString() + "\n" + "Raw Speed: " + InternetConnectionState::RawSpeed + "\n";
		});
	}
	else
	{
		TextBoxResults->Text = "Not connected...";
	}

