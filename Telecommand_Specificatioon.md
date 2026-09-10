# Telecommand specificatioon
The floatsat commands will be sent from the ground station as Unix-style 
console commands. Even though the student uses a graphical user interface (GUI),
the ground station creates these commands under the hood and sends them to the 
floatsat  

The Unix style is adopted becuase its a ubiquitous standard that is familiar to 
many programmers. 

## About the format
(Based on the Argtable3 syntax)

The command itself is the first word. All the other text bits are its arguments.
Arguments can require a value or not. For example, if the command has an argument 
for the number of lines to print, we could express it as `--lines=5` or `-l 5`.
If the argument does not require a value, it is called an option. For example, 
we could activate verbose output with `--verbose` of `-v` 

The name of arguments can be written in two forms, which we'll call long 
or short. The long name is prepended by `--`, while the short name is prepended 
by `-`. 


`[...]` means that the argument/option is, well, optional.  
`<...>` means a placeholder for a value you have to write yourself.  
An argument that appears literally, like `add` in `git add`, is called a literal. 
This represents a subcommand or fixed keyword and is written exactly as typed, without
any prefix or delimiters. They are also positional, meaning that they must appear
in the same order as documented.


## Commands 
### `control` - Manage control paremeters

#### Format
`control [-i]` 
`control [-t <type>] -p <params>...`

#### Description
`-i, --info`  
Shows the information of the current controller, this includes controller type 
and parameterd

`-t <type>, --type=<type>`  
Control model to use, specified as a string. Available options are: `PID`, `LQR`

`-p <params>, --params=<params>`  
Parameters of the controller, specified as list of decimal numbers separated by comma.
The number and order of elements must correspond to the parameters of the current 
controller.  
For PID: `P I D`  
For LQR:  (To be defined)

___

### `mission` - Select current mission

#### Format
`mission [-i]` 
`mission <mission_id>`

#### Description
`-i, --info`  
Shows the current mission and its status (if applies).

`<mission_id>`  
Specifies the ID of the mission to change into.

___

### `photo` - Manage photo command sequence
#### Format
`photo -i`  
`photo add -a <angle> -t <time> -d <duration>`  
`photo reset`

#### Description
`-i, --info`  
Shows the currently stored photo sequence

`add`  
Add a new entry to the photo sequence

`-a <angle>, --angle=<angle>`  
Set the angle azimuth angle of the photo, in degrees. Specified as a decimal 
number in the range [0, 359.9]  

`-t <time>, --time=<time>`  
Set the time of the photo. Specified in the format HH:mm:ss of Standard Atlantic 
Time (UTC-4)  

`-d <duration>, --duration=<duration>`  
Set the duration of the photo. Specified as milliseconds

`reset`  
Clear the stored photo sequence