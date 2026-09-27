# tinyping

A lightweight uptime monitor for homelabs where resources are scarce.

tinyping checks whether your hosts are reachable and sends notifications through [ntfy.sh](https://ntfy.sh)

## Why tinyping?

I originally wrote this project in Python but my intended deployment target, a Raspberry Pi Zero W, made memory usage a priority. I rewrote it in C++ to reduce its footprint and make better use of the limited resources available.

In my testing, the C++ implementation uses approximately **16 MB of memory on average**, roughly half the footprint of my original Python implementation. Your results may vary depending on your system and configuration.

## How it works

tinyping uses ICMP ping requests to check whether configured hosts are reachable. It integrates with ntfy to deliver monitoring notifications to your phone.

The notification setup is straightforward:

1. Install the ntfy mobile app.
2. Subscribe to the topic you want to use for tinyping.
3. Configure tinyping to publish to that topic. (more on this below)

tinyping sends notifications by making HTTP POST requests to the configured ntfy topic. ntfy then delivers those messages to subscribed devices.

A successful ping confirms that a host responds to ICMP; it does not guarantee that every application or service running on that host is working.

## Compile and service installation

in the repo are two scripts, one for compiling and installing necessary dependencies. The other is for installing the application as a systemd service (sorry Linux fans).

### 1. clone the repo to your machine
```bash
git clone https://github.com/daemon-c/tinyping.git
```
### 2. make build.sh and install-tinyping-service.sh executable
```bash
sudo chmod +x build.sh && sudo chmod +x install-tinyping-service.sh
```
### 3. After confirming the contents of the scripts yourself, run them to build, compile, and install the service
```bash
sudo ./build.sh && sudo ./install-tinyping-service.sh
```
### 4. Edit the configuration file for tiny ping
Located in /var/opt/tinyping/tinyping.conf
it should be initialized with the following content
```
# Enter IP addresses or hostnames below.
# One IP or hostname per line.
# Set your ntfy topic after the > symbol.
> enter_ntfy_topic_here
8.8.8.8
```
replace enter_ntfy_topic_here with your generated topic (recommend making something long / unique / hard to just guess. Anyone who has the string you create as a topic can get these alerts in ntfy)
enter one host per line, save it
### 5. Restart the tinyping service
```bash
sudo systemctl restart tinyping.service
```
voilà, a lightweight way to make sure a machine is online!

## The future
- Planning on adding windows support and proper macOS support
- not planning on adding any gui or web application front ends to this. I want it to stay really simple. Personally been using this on a zero2w connected to a battery backup with my router, really helpful if a machine or my entire internet goes down

## Pull requests
feel free to open one. Ill take a look when I have time, appreciate everyones time reading this and trying the software out!

## Built with

- **C++17**
- **cpp-icmplib** for ICMP ping requests
- **libcurl** for HTTP requests
- **ntfy** for notification delivery
