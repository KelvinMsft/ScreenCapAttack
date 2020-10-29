# ScreenCap

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/249829480c0c4acfbd5cced6c480d0e3)](https://app.codacy.com/gh/Kelvinhack/ScreenCapAttack?utm_source=github.com&utm_medium=referral&utm_content=Kelvinhack/ScreenCapAttack&utm_campaign=Badge_Grade)

- By using a Mirror Driver technique for making a kernel level screen capture.

# Compile Environment
- Visual Studio 2015 with update 3 </br>
- Windowr Driver Kit 7 or above</br>
- MFC library

# Runtime Environment
- Windows 2000 to Windows 7 for x64 (also compatible with x86)

# Description
- ScreenCap is a MFC DLL and packed with a miniport driver(mirror driver) and related file into a resource, </br>
  ScreenCap provided a exported function that can be able to make a kernel-level Screen Capture on Windows 2000 </br>
  to Windows 7 for x86/x64 platform, and which is using a mini-port driver technique, by attached to videoprt.sys.
  And the driver will map the memory which is pointer to self-created surface it will be updated by intercepting GDI rendering.
  
# Compile
 1. Build a Mini-port driver and user-mode driver by WDK, then we will get a <l><b>Mirror.dll</b> </l>/<l><b> Mirror.sys</b></l>
 2. Put them into bin-x64
 3. Compile a <l><b> ScreenCap.dll </b></l>, it will pack the files into resources.
 4. Compile a <l><b>Tester.exe </b></l>, test a screen capture functions.
 
# Demo Program: 
  - <b>Tester.exe</b> by pressing default command '2', then the driver will be loaded by default INF file <br/>
    and finally a demo program will make a screenshot which will be saved in <b> C:\CopyScreen2.bmp </b> file.

# REMARKS
- This project is just for experiment, and I suggest it run in Virtualization Environment. 
  Since the INF file default the Driver will be loaded during boot time. 
