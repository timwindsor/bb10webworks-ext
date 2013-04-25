# Virtual Keyboard API

This extension provides additional APIs to control the virtual keyboard.

With the exception of the physical keyboard detection call the functionality of this API is only useful on touch-only devices as physical keyboard devices do not employ the Virtual Keyboard.

It should be noted that the system VK handler will alter the layout when, for example, a text input field gains focus and may hide the VK when losing focus

**Tested On**

* BlackBerry 10.0.9.2372 (Z10 LE)
* BlackBerry 10.1.0.1483 (Dev Alpha C)

**Applies To**

* [BlackBerry 10 WebWorks SDK](https://developer.blackberry.com/html5/download/sdk) 

**Author(s)** 

* [Peardox] (http://supportforums.blackberry.com/t5/user/viewprofilepage/user-id/325249)

## Set up

Copy community.VKeyboard into the Framework\ext folder of the WebWorks SDK

## Use

<pre>
// Check if the device has a Physical Keyboard
// Returns 0 (Touch only) or 1 (Has real keyboard)
community.VKeyboard.VKhasPhysicalKeyboard()
</pre> 

<pre>
// Show the Virtual Keyboard
community.deviceInfo.VKshow()
</pre>	

<pre>
// Hide the Virtual Keyboard
community.deviceInfo.VKhide()
</pre>	

<pre>
// Set the Virtual Keyboard layout
var VKLayout = { DEFAULT=0, URL=1, EMAIL=2, WEB=3, NUM_PUNC=4, SYMBOL=5, PHONE=6, PIN=7, PASSWORD=8, DIAL_PAD=9 };
var VKEnter = { DEFAULT=0, GO=1, JOIN=2, NEXT=3, SEARCH=4, SEND=5, SUBMIT=6, DONE=7, CONNECT=8 };

e.g. community.deviceInfo.VKsetLayout(VKLayout.PASSWORD, VKEnter.CONNECT);
</pre>	

## Currently not working properly

<pre>
// Get the Virtual Keyboard height
// Returns height in pixels
community.deviceInfo.VKgetHeight()
</pre>	

<pre>
// Events
community.VKeyboard.VKvisible
community.VKeyboard.VKhidden
community.VKeyboard.VKchangeHeight
</pre>

## Sample Application

The simple sample application in vkbtest shows all functionality on a touch-only device, note that the first four keyboard layouts only change one key each (left of space)

On a keyboard equipped device it'll tell you it's got a keyboard, nothing else has any effect

## Contributing Changes

Please see the [README](https://github.com/blackberry/WebWorks-Community-APIs) of the WebWorks-Community-APIs repository for instructions on how to add new Samples or make modifications to existing Samples.


## Bug Reporting and Feature Requests

If you find a bug in a Sample, or have an enhancement request, simply file an [Issue](https://github.com/blackberry/WebWorks-Community-APIs//issues) for the Sample and send a message (via github messages) to the Sample Author(s) to let them know that you have filed an [Issue](https://github.com/blackberry/WebWorks-Community-APIs//issues).

## Disclaimer

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.