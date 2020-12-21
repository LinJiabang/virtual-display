# virtual-display
1. What is this? 用途解釋

   This sample code demonstrates how to make a  USB or Ethernet display product
   by implementing a kernel driver that talks to LCI Proxy WDDM driver.

   LCI Proxy WDDM driver is a filter driver that talks to inbox display WDDM
   driver, and creates a virtual monitor on behalf of inbox display driver.

   The USB part of the driver receives the surface create/destroy/update messages
   from LCI Proxy WDDM driver. This sample code demonstrates  the interaction
   between the source (LCI Proxy WDDM driver) and  the target (eg. USB/Ethernet
   kernel driver) so that driver developers can make a USB/Ethernet display
   product easily.
   
   此範例是教導如何製作一個內核(kernel)驅動來實現 USB 或 Ethernet 顯示器產品。 此範例須利用 LCI Proxy WDDM 驅動來協助完成。
   LCI Proxy WDDM 驅動是一個 filter 驅動，用於凌駕原始顯示驅動，使得 Windows 系統能列舉生成一個虛擬顯示器。LCI Proxy WDDM 驅動在這裡僅當作教學用途，不能當作商業使用。** 任何商業使用　LCI Proxy WDDM 驅動需經合法授權。非法使用 LCI Proxy WDDM 驅動者，後果自負。**
   
   在這裡 USB/Ethernet 驅動所扮演的腳色是用來接收從 LCI Proxy WDDM 來的屏幕生成／消失訊息。此範例解釋 LCI Proxy WDDM 驅動與　USB/Ethernet 驅動之間的溝通關係，以利方便實現 USB/Ethernet 顯示器產品。

2. Target audience 讀者區分

   The sample code is targeted to USB/Ethernet display kernel driver developers.
   The sample code illustrates the internal driver communication between
   LCI Proxy WDDM driver and a target USB/Ethernet ethernet driver.

   The lci_display_internal_ioctl.h defines the internal IOCTL codes so that the
   target driver could get the virtual screen content efficiently.
   
   The ljb_vmon_ioctl.h is used by user mode sample app so that you can see the
   content of the virtual screen.
   
   The USB/Ethernet display developers can implement a real kernel device driver
   by leveraging the source code.
   
   The "notify" folder is derived from Microsoft toaster sample code, and is
   used merely fro demo purpose only. The user mode app is not intended to be 
   used for real product scenario.

   此範例是給 USB/Ethernet 顯示器產品開發者所設：
   
   lci_display_internal_ioctl.h 定義內核溝通代碼(internal IOCTL code)，是產生虛擬屏幕的必要條件。
   ljb_vmon_ioctl.h 適用於 user mode 程式，用於顯示屏幕內容。
   
   開發者可利用此範例程式碼來實作內核驅動。
   
   notify 項目是從 Microsoft toaster 範例所衍生，僅用於教學用途，不適用於實際產品。

3. How to compile code.

   You need to install WDK10 as well WDK7. The user mode sample app is compiled 
   with WDK7, while the kernel driver is compiled by WDK10.
   
   Use buildall_wdk10.cmd to build all components. An output folder is created
   as the result.

   There is also a precompiled driver binary under precompiled folder.

4. How to run the demo

   4.1 First, you need to install LCI Proxy WDDM driver. The LCI Proxy WDDM driver can be found in Fresco Logic FL2000 driver package.
       Goto https://support.frescologic.com/portal/home , and find the FL2000-2.1.xxxx.0 on the right hand side.

   4.2 Install the FL2000 driver even if you don't have FL2000 USB dongle. After installation, navigate to
       C:\Program Files\Fresco Logic\Fresco Logic USB Display Driver folder. Note that there is a devcon and lci_proxykmd folder.
       Copy out those 2 folders as you will need them later.

   4.3 Uninstall the FL2000 driver after step 4.2.

   4.4 Install lci_proxykmd by invoking cmd.exe with administrator privilege.
       Use the following command to install lci_proxykmd driver:
       "devcon\x64\devcon.exe install lci_proxykmd\lci_proxykmd.inf  root\lci_proxykmd"
  
      You will see a "Fresco Logic Proxy Display Adapter" device node in the device manager view. You can also use "devcon\x64\devcon.exe status root\lci_proxykmd" to verify that it is correctly installed.  Then you need to do "devcon\x64\devcon.exe restart =display"  to restart  display driver (such as Intel Graphics).

   4.5 Goto the output folder generated in step 3. In the cmd.exe window, navigate to the folder where "vmon_func.inf" exists.
       Use "devcon.exe install vmon_func.inf  root\ljb_vmon" to create a ljb_vmon device node.

   4.6 Invoke vmon.exe. You will see an extra monitor is created in device manager view. Also the vmon.exe displays the content
       on the virtual monitor just created.
