# Screen Display API

This extension provides additional APIs supplying information about the device screen display.

**Tested On**

* BlackBerry 10.1.0.1483 (Z10 LE)
* BlackBerry 10.1.0.1483 (Dev Alpha C)

**Applies To**

* [BlackBerry 10 WebWorks SDK](https://developer.blackberry.com/html5/download/sdk) 

**Author(s)** 

* [Peardox] (http://supportforums.blackberry.com/t5/user/viewprofilepage/user-id/325249)

## Set up

Copy community.ScreenDisplay into the Framework\ext folder of the WebWorks SDK

## Use

<pre>
// Get information about the screen
var sd = community.ScreenDisplay.SDgetSize()

Return value is an object with the following properties
 
pixelWidth,      // Pixel Size - width
pixelHeight,     // Pixel Size - height
physicalWidth,   // Physical Size mm - width
physicalHeight,  // Physical Size mm - height
ppmm,            // Pixels Per mm
ppmmX,           // Pixels Per mm - X
ppmmY,           // Pixels Per mm - Y
ppi,             // Pixels Per Inch
ppiX,            // Pixels Per Inch - X
ppiY,            // Pixels Per Inch - Y
pixelShape;      // Physical Shape

Notes

The first four numbers are returned from the device. The other numbers are calculates from these four base readings.

The device may return numbers indicating non-square pixels if wither the physical sizes are not accurate or the device truly has non-square pixels.

The ppmm and ppi values are calculated from the screen diagonal to get one easy number

The pixelShape (aspect ratio) figure is based calculated figures and so may be slightly off as well.

pixelShape = 1 means square, 2 means twice as wide as high, 0.5 means twice as wide as high (it is doubtful you'll ever see anything far from 1)

</pre>	

## Sample Application

The simple sample application in sdtest shows all functionality

## Contributing Changes

Please see the [README](https://github.com/blackberry/WebWorks-Community-APIs) of the WebWorks-Community-APIs repository for instructions on how to add new Samples or make modifications to existing Samples.


## Bug Reporting and Feature Requests

If you find a bug in a Sample, or have an enhancement request, simply file an [Issue](https://github.com/blackberry/WebWorks-Community-APIs//issues) for the Sample and send a message (via github messages) to the Sample Author(s) to let them know that you have filed an [Issue](https://github.com/blackberry/WebWorks-Community-APIs//issues).

## Disclaimer

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.