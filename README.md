# Annotator Daemons

Annotator daemons that runs in the OS environment. Currently there are three separate processes - **HttpDataReSender**, **InterfaceMonitor** and **Joiner** which resides in separate folder. These processes uses some common functionality, that is being located in *Utils* folder. Definition of the messages, that are being exchanged between the proccesses are located in *ProtobufMessages* folder. 

Currently, only supported OSes are those based on the UNIX core (linux, macOS) and supported architectures are: amd64 and arm64 (aarch64).

## Dependencies
All of the daemons depends on these three libraries:

- [libpcap](https://www.tcpdump.org)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)
- [nlohmann-json](https://github.com/nlohmann/json)

Please follow the instructions on the website of each library to install those. All of the should be availalble through package managers (aptitude, brew,...).

## Build
You can build all the tools together using `build.sh` script, that creates *dist* folder containing compiled binaries and configuration file, or you can build each tool seprately.

### `build.sh`
Easiest way to compile al of the deamons. This tool creates *build* folder in subdirectories (excluding *Utils* and *Tools*) and there it will run `cmake ..`. After successfull compilation, it will create needed folder structure in *dist* directory and copies the output binaries there. 

After succesfull run, you can grab the whole *dist* directory and move it to your desired location for easier use.

## Separate compilation
Each subdirectory is a separate CMake project that can be compiled independently. There is only one exception - *ProtobufMessages* folder, that is being shared with all of the projects. In this directory **you must create** build folder with name **`build`** and no other name. This is because the files in the build folder are included in the sources of other daemons. 

With this compulsory step in mind, you can just do this fol all the subdirectories:
```
$ cd {subdirectory}
$ mkdir build
$ cd build
$ cmake ..
$ make
```
This will create binary file in the build folders. 

## Execution
Each tool can be configured using `configure.json` file that is located like this:
```
{base dir}
 | config.json
 | {directory with binaries}
    | HttpDataReSender
    | InterfaceMonitor
    | Joiner
```
Inside of this file, there is a common section (usually just path where to log) and then each tool has its own section with own settings. There is a template that contains all available options. 
```
{
  "logFilePath" : "default",
  "InterfaceMonitor":{
    "domainSocketPath" : "/tmp/IFMonitor"
  },
  "HttpDataReSender" : {
    "domainSocketPath" : "/tmp/HttpDataReSender"
  },
  "Joiner" : {
    "dispatcherIp" : "127.0.0.1",
    "dispatcherPort" : 50555,
    "archivePath" : "/usr/user/archive.json"
  }
}
```
After you specify the configuration that you want, tools need to run in this order: 

1. Joiner - Must run first. Creates sockets for receiving messages from others.
1. InterfaceMonitor - Recomended to run second, but can be ran as last as well.
1. ~~HttpDataReSender~~ - This tool won't be executed by you, but by browser. You can control this by reloading the Browser Extension (see [Browser Extension](https://github.com/jan-kala/BrowserExtension) repository for more information).

When you successfully start these tools, outputs are these: 

- History of paired flows with web requests will be saved into file specified by `"archivePath"` parameter in the configuration file.
- Joiner will print active connections on `stdout`

And also there will be open TCP port of dispatcher, which you can use for pulling the data from the Dispatcher. For this purpose, you can use [FlowAnnotationRequests.py](./Tools/FlowAnnotationRequests.py) script.

### Logging
If you omit the `logFilePath`, tools will try to log to the `stdout`, **BUT BE CAUTIOUS** that this can break the HttpDataResender that uses `stdout` for communication. 

If you leave the path set to `"default"` loggs will be saved to the *log* dir that resides as a subdir of the parent directory like this: 
```
{base dir}
 | log
    | InterfaceMonitor.csv
 | {directory with binaries}
    | InterfaceMonitor 
```
If you delete the `log` the directory, no logs will be saved. 

