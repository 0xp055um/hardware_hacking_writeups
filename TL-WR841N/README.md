# Overview
My attempt to hack the TL-WR841N V14 from Tp-link.
This isn't by any means exhaustive and there are still a lot more things you can do to further exploit it.

At an overview we managed to:
- Gather OSINT
- Connect via unrestricted/unauthenticated UART
- Upload and download files
- Dump the firmware
- Analyse it locally on our computer

## Contents
- [OSINT](#OSINT)
- [UART](#UART)
   - [Enumeration](#Enumeration)
   - [Upload](#Upload)
- [Firmware Dumping](#Firmware-Dumping)
- [Static Analysis](#Static-Analysis)


# OSINT
Osint (Open-source Intelligence) is about finding any publicly available information on our target device on the internet.

Luckily the FCC forces every company that wants to sell electronic devices in America to disclose some information publicly, such as external and internal photos.

The website [fccid.io](https://fccid.io/) catalogs a lot of products and is an invaluable resource for that reason.

Searching specifically for our model and version ([link](https://fccid.io/TE7WR841NV14)) we find the user manual, but most importantly we get pictures of the entire PCB, the layout and its internal parts, such as the model of the MCU, MT7628NN [link](https://fccid.io/TE7WR841NV14/Internal-Photos/TE7WR841NV14-Inphoto-3687060), but even more interesting is the VCC, GND, RX and TX on the side of the PCB that indicate the presence of UART

![UART from fccid](Attachments/20240907084259.png)
![MCU](https://cdn-0.fccid.io/png.php?id=3687060&page=2)

Searching about the model MT7628NN online we find that the CPU is MIPS architecture
![CPU specs](Attachments/20240624183324.png)


# UART
Opening up the TP-LINK TL-WR841N and taking a look at the PCB we see the MCU model and the UART port we already discovered from fccid.

![close up of the UART](Attachments/_20240907_081714.jpg)

 Using a TTL to usb converter we can connect the RX and TX of the UART with a computer and using minicom or picocom with the USB device that connects to the UART we can interface with it.
![UART with the cables](Attachments/_20240907_082140.jpg)
![minicom shell](Attachments/20240624175227.png)

The UART has no authentication and doesn't place you in a restrictive shell or any sandbox environment. Instead we are greeted with a root shell.

![Getting the shell](Attachments/20240624175313.png)

Our regular computers have binaries for each Unix command that we use, ls, pwd, id, whoami, etc.
On embedded devices things are different. There are many hardware limitation and developers need to be more careful about the resources used.
Busybox is a single binary that can be configured to only have the commands that your embedded device needs.

In our case the busybox that they included is very restrictive and limiting. 
![Default Busybox](Attachments/20240624175151.png)

If we had a way to transfer files to the device over the network we could upload a busybox binary that includes a lot more commands and make our exploitation a lot easier. Thats where tftpd comes in, but more on that later.

Usually in embedded devices not every directory is writable. We would need to identify the location of any volatile memory.

Using the "mount" command (luckily busybox includes it) we can see that the `/var` uses ramfs which means that this will be the partition where we can drop our binaries.

![mount](Attachments/20240624175341.png)

But before that, lets enumerate and see what we got.

## Enumeration
Enumerating the various directories and files in the system we come across the hash for the administrator (hardcoded credentials) and the default wifi password.

![etc](Attachments/20240624175443.png)

The contents of passwd file:

![passwd](Attachments/20240624175516_1.png)

the contents of RT2860AP.dat file:

![RT2860AP.dat](Attachments/20240624182010_1.png)

The network services that are running and on which port.
![netstat](Attachments/20240624180135.png)

Taking a look under the `/var` directory we find a lot of interesting files such as a dropbear key and the upnp configuration files.
![var](Attachments/20240624180835.png)

![dropbear](Attachments/20240624180817.png)

![upnp](20240624181013.png)

![More upnp](Attachments/20240624181053.png)

## Upload
From our enumeration we saw that the default busybox had a command called tftpd.

Looking at the documentation for tftp online we see that we cant transfer files to and from the router.
More specifically by using the "g" flag we can "get" a (r)emote file from our computer, to the router device.

![tftp manpage](Attachments/20240624184708.png)

Back in our computer we initialize the tftp service and move the desired file that we want to upload to the `/var/lib/tftpboot` directory.
![starting tftpd](Attachments/20240624184915.png)
![tftpboot](Attachments/20240624184930.png)

Note that we do need to use the right busybox for our architecture, in our case its mips as we found out from our [OSINT](#OSINT).

In our UART shell we need to provide the name of the file we want to "get" and the IP of the host to get it from.
![tftp get](Attachments/20240624185129.png)

We can also use the "p" flag to "put" a (l)ocal file to another device, again, our host, which allows us to transfer the dropbear rsa key to our computer.

![tftp put](Attachments/20240624193303.png)

Finally we can use our new busybox binary with all the typical Unix commands we know and love
![New Busybox](Attachments/20240624185348.png)

![caption](Attachments/20240624200241.png)


## Backdoor
We can set up a [bind_shell](https://github.com/lilithgkini/Malware_Development/blob/main/Bind_Shell) for persistence on the device. 

Since we are working with mipsel we need to compile it for that architecture
![compile](Attachments/20240922154257.png)

Then we can upload the backdoor binary to the router via tftp.
![Uploading the shell](Attachments/20240922145143.png)

run `chmod +x backdoor` and execute it.
![Setting up the shell](Attachments/20240922145405.png)

Finally back in our machine we can use netcat to connect to the target and get a shell.
![Getting the shell](Attachments/20240922145447.png)

# Firmware Dumping
Another thing we haven't mentioned so far is the EEPROM right next to the UART.
Taking a closer look we see that it says "cFeon Q32B-104".
Searching online we come across the [data sheet](https://www.alldatasheet.com/datasheet-pdf/pdf/458184/EON/EN25Q32B-104HIP.html) for this model which tells us that It's communicating with the MCU via SPI bus and shows us exactly which parts are the MISO, MOSI, clock and chip select.

![datasheet for flash](Attachments/20240907102208.png)

Connecting our test hooks with the appropriate legs and to a raspberry pi pico (you can use any microcontroller you prefer, as long as you load it with the right script to read the flash) we can dump the firmware by running 
```bash
sudo flashrom -p serprog:dev=/dev/ttyACM0,spispeed=104000 -c W25Q64BV/W25Q64CV/W25Q64FV -r firmware.bin
```
![dumping the firmware](Attachments/_20240907_081355.jpg)


# Static Analysis
Once the process finishes we can use binwalk to extract the filesystem and analyze it.
```bash
binwalk firmware.bin
cd _firmware.bin.extracted
```

Listing all the files we see the passwd file with the hash of the admin and also the RT2860AP.dat that has the default password for the WiFi, just like we saw in the UART shell.

![static analysis](Attachments/20240907085900.png)

Running file on the busybox binary we can see again that the architecture is MIPS Least Significant Byte.
![busybox architecture](Attachments/20240907164145.png)
