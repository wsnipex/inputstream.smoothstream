# inputstream.smoothstream

This is a SmoothStreamingMedia file addon for kodi's new InputStream Interface.

- this addon is part of the official kodi repository and part of each kodi installation
- configure the addon by adding URL prefixes wich are allowed to be played by this addon
- Open a .ism file on your local filesystem
- or create a .strm file / or addon with passes an url with .ism extension and open the strm file in kodi
- or write an addon wich passes .ism files to kodi

##### Examples:
tbd.

##### Decrypting:
Decrypting is not implemented. But it is prepared!  
Decrypting takes place in separate decrypter shared libraries, wich are identified by the inputstream.smoothstream.licensetype listitem property.  
Only one shared decrypter library can be active during playing decrypted media. Building decrypter libraries do not require kodi sources.  
Simply check out the sources of this addon and you are able to build decrypters including full access to existing decrypters implemented in bento4.

##### TODO's:
- Adaptive bitrate switching is prepared but currently not yet activated  
Measuring the bandwidth needs some intelligence to switch not for any small network variation
- Automatic / fixed video stream selection depending on max. visible display rect (some work has to be done at the inputstream interface). Currently videos > 720p will not be selected if videos <= 720p exist.
- Currently always a full segment is read from source into memory before it is processed. This is not optimal for the cache state - should read in chunks.
- There will be a lot of SmoothStreamingMedia implementations with unsupported xml syntax - must be extended. 

##### Notes:
- On startup of a new video the average bandwidth of the previous played stream is used to choose the representation to start with. As long bandwith measurement is not fully implemented this value is fixed 4MBit/s and will not be changed.  This value can be found and modified in settings.xml, but you can also override this value using Min/Max bandwidth in the settings dialog for this addon.
- The URL entries in the settings dialog support regular expressions, please note that at least 7 chars must match the regexp to be valid. Example: http://.*.videodownload.xy supports all subdomains of videodownload.xy
- This addon is single threaded. The memory consumption is the sum of a single segment from each stream currently playing (will be reduced, see TODO's) Refering to known streams it is < 10MB for 720p videos.
- Win32 users: There is a bug in the curl library wich comes with the current kodi version. Until curl is not upgraded sometimes loading a .ism file fails.

##### Credits:
[@fernetmenta](github.com/fernetmenta) Best support I ever got regarding streams / codecs and kodi internals.  
[@notspiff](https://github.com/notspiff) Thanks for your ideas / tipps regarding kodi file system  
[bento4 library](https://www.bento4.com/) For me the best library choice for mp4 streams. Well written and extensible!

##### Continuous integration:
[Travis CI build state:](https://travis-ci.org/mapfau) ![alt tag](https://travis-ci.org/mapfau/inputstream.smoothstream.svg?branch=master)  
