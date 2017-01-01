# ScreenCapAttack
- By using a Mirror Driver technique for making a kernel level screen capture.

#Compile Environment
- Visual Studio 2015 update 3 </br>
- Windows SDK 10</br>
- Windowr Driver Kit 10</br>


#Runtime Environment
- Windows2000 to Windows7 for x64 (also comptible with x86)

#Description
- ScreenCap is a MFC DLL and packed with a miniport driver(mirror driver) and related file into a resource, </br>
  ScreenCap provided a exported function that can be able to make a kernel-level Screen Capture on Windows 2000 </br>
  to Windows 7 for x86/x64 platform, and which is using a mini-port driver technique, by attached to videoprt.sys.

Demo Program: 
<b>Tester.exe</b> by pressing default command '2', then the driver will be loaded by default INF file <br/>
and finally a demo program will make a screenshot which will be saved in <b> C:\CopyScreen2.bmp </b> file.

