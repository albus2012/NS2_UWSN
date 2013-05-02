static char code[] = "\n\
if {[info commands package] == \"\"} {\n\
error \"version mismatch: library\\nscripts expect Tcl version 7.5b1 or later but the loaded version is\\nonly [info patchlevel]\"\n\
}\n\
package require -exact Tcl 8.4\n\
\n\
\n\
if {![info exists auto_path]} {\n\
if {[info exists env(TCLLIBPATH)]} {\n\
set auto_path $env(TCLLIBPATH)\n\
} else {\n\
set auto_path \"\"\n\
}\n\
}\n\
namespace eval tcl {\n\
variable Dir\n\
if {[info library] ne \"\"} {\n\
foreach Dir [list [info library] [file dirname [info library]]] {\n\
if {[lsearch -exact $::auto_path $Dir] < 0} {\n\
lappend ::auto_path $Dir\n\
}\n\
}\n\
}\n\
set Dir [file join [file dirname [file dirname \\\n\
[info nameofexecutable]]] lib]\n\
if {[lsearch -exact $::auto_path $Dir] < 0} {\n\
lappend ::auto_path $Dir\n\
}\n\
if {[info exists ::tcl_pkgPath]} {\n\
foreach Dir $::tcl_pkgPath {\n\
if {[lsearch -exact $::auto_path $Dir] < 0} {\n\
lappend ::auto_path $Dir\n\
}\n\
}\n\
}\n\
}\n\
\n\
\n\
if {(![interp issafe]) && $tcl_platform(platform) eq \"windows\"} {\n\
namespace eval tcl {\n\
proc EnvTraceProc {lo n1 n2 op} {\n\
set x $::env($n2)\n\
set ::env($lo) $x\n\
set ::env([string toupper $lo]) $x\n\
}\n\
proc InitWinEnv {} {\n\
global env tcl_platform\n\
foreach p [array names env] {\n\
set u [string toupper $p]\n\
if {$u ne $p} {\n\
switch -- $u {\n\
COMSPEC -\n\
PATH {\n\
if {![info exists env($u)]} {\n\
set env($u) $env($p)\n\
}\n\
trace add variable env($p) write \\\n\
[namespace code [list EnvTraceProc $p]]\n\
trace add variable env($u) write \\\n\
[namespace code [list EnvTraceProc $p]]\n\
}\n\
}\n\
}\n\
}\n\
if {![info exists env(COMSPEC)]} {\n\
if {$tcl_platform(os) eq \"Windows NT\"} {\n\
set env(COMSPEC) cmd.exe\n\
} else {\n\
set env(COMSPEC) command.com\n\
}\n\
}\n\
}\n\
InitWinEnv\n\
}\n\
}\n\
\n\
\n\
package unknown tclPkgUnknown\n\
\n\
if {![interp issafe]} {\n\
if {$::tcl_platform(platform) eq \"unix\"\n\
&& $::tcl_platform(os) eq \"Darwin\"} {\n\
package unknown [list tcl::MacOSXPkgUnknown [package unknown]]\n\
}\n\
if {$::tcl_platform(platform) eq \"macintosh\"} {\n\
package unknown [list tcl::MacPkgUnknown [package unknown]]\n\
}\n\
}\n\
\n\
\n\
if {[namespace which -command exec] eq \"\"} {\n\
\n\
\n\
set auto_noexec 1\n\
}\n\
set errorCode \"\"\n\
set errorInfo \"\"\n\
\n\
\n\
if {[namespace which -command tclLog] eq \"\"} {\n\
proc tclLog {string} {\n\
catch {puts stderr $string}\n\
}\n\
}\n\
\n\
\n\
proc unknown args {\n\
global auto_noexec auto_noload env unknown_pending tcl_interactive\n\
global errorCode errorInfo\n\
\n\
\n\
set cmd [lindex $args 0]\n\
if {[regexp \"^:*namespace\\[ \\t\\n\\]+inscope\" $cmd] && [llength $cmd] == 4} {\n\
set arglist [lrange $args 1 end]\n\
set ret [catch {uplevel 1 ::$cmd $arglist} result]\n\
if {$ret == 0} {\n\
return $result\n\
} else {\n\
return -code $ret -errorcode $errorCode $result\n\
}\n\
}\n\
\n\
\n\
if {![info exists errorCode]} {\n\
set errorCode \"\"\n\
}\n\
if {![info exists errorInfo]} {\n\
set errorInfo \"\"\n\
}\n\
set savedErrorCode $errorCode\n\
set savedErrorInfo $errorInfo\n\
set name $cmd\n\
if {![info exists auto_noload]} {\n\
if {[info exists unknown_pending($name)]} {\n\
return -code error \"self-referential recursion in \\\"unknown\\\" for command \\\"$name\\\"\";\n\
}\n\
set unknown_pending($name) pending;\n\
set ret [catch {auto_load $name [uplevel 1 {::namespace current}]} msg]\n\
unset unknown_pending($name);\n\
if {$ret != 0} {\n\
append errorInfo \"\\n    (autoloading \\\"$name\\\")\"\n\
return -code $ret -errorcode $errorCode -errorinfo $errorInfo $msg\n\
}\n\
if {![array size unknown_pending]} {\n\
unset unknown_pending\n\
}\n\
if {$msg} {\n\
set errorCode $savedErrorCode\n\
set errorInfo $savedErrorInfo\n\
set code [catch {uplevel 1 $args} msg]\n\
if {$code ==  1} {\n\
set cinfo $args\n\
set ellipsis \"\"\n\
while {[string bytelength $cinfo] > 150} {\n\
set cinfo [string range $cinfo 0 end-1]\n\
set ellipsis \"...\"\n\
}\n\
append cinfo $ellipsis \"\\\"\\n    (\\\"uplevel\\\" body line 1)\"\n\
append cinfo \"\\n    invoked from within\"\n\
append cinfo \"\\n\\\"uplevel 1 \\$args\\\"\"\n\
set expect \"$msg\\n    while executing\\n\\\"$cinfo\"\n\
if {$errorInfo eq $expect} {\n\
return -code error -errorcode $errorCode $msg\n\
}\n\
set expect \"\\n    invoked from within\\n\\\"$cinfo\"\n\
set exlen [string length $expect]\n\
set eilen [string length $errorInfo]\n\
set i [expr {$eilen - $exlen - 1}]\n\
set einfo [string range $errorInfo 0 $i]\n\
if {$errorInfo ne \"$einfo$expect\"} {\n\
error \"Tcl bug: unexpected stack trace in \\\"unknown\\\"\" {} \\\n\
[list CORE UNKNOWN BADTRACE $expect $errorInfo]\n\
}\n\
return -code error -errorcode $errorCode \\\n\
-errorinfo $einfo $msg\n\
} else {\n\
return -code $code $msg\n\
}\n\
}\n\
}\n\
\n\
if {([info level] == 1) && [info script] eq \"\" \\\n\
&& [info exists tcl_interactive] && $tcl_interactive} {\n\
if {![info exists auto_noexec]} {\n\
set new [auto_execok $name]\n\
if {$new ne \"\"} {\n\
set errorCode $savedErrorCode\n\
set errorInfo $savedErrorInfo\n\
set redir \"\"\n\
if {[namespace which -command console] eq \"\"} {\n\
set redir \">&@stdout <@stdin\"\n\
}\n\
return [uplevel 1 exec $redir $new [lrange $args 1 end]]\n\
}\n\
}\n\
set errorCode $savedErrorCode\n\
set errorInfo $savedErrorInfo\n\
if {$name eq \"!!\"} {\n\
set newcmd [history event]\n\
} elseif {[regexp {^!(.+)$} $name -> event]} {\n\
set newcmd [history event $event]\n\
} elseif {[regexp {^\\^([^^]*)\\^([^^]*)\\^?$} $name -> old new]} {\n\
set newcmd [history event -1]\n\
catch {regsub -all -- $old $newcmd $new newcmd}\n\
}\n\
if {[info exists newcmd]} {\n\
tclLog $newcmd\n\
history change $newcmd 0\n\
return [uplevel 1 $newcmd]\n\
}\n\
\n\
set ret [catch {set candidates [info commands $name*]} msg]\n\
if {$name eq \"::\"} {\n\
set name \"\"\n\
}\n\
if {$ret != 0} {\n\
return -code $ret -errorcode $errorCode \\\n\
\"error in unknown while checking if \\\"$name\\\" is\\\n\
a unique command abbreviation:\\n$msg\"\n\
}\n\
if {$name eq \"\"} {\n\
if {[llength $candidates] != 1} {\n\
return -code error \"empty command name \\\"\\\"\"\n\
}\n\
return [uplevel 1 [lreplace $args 0 0 [lindex $candidates 0]]]\n\
}\n\
set cmds [list]\n\
foreach x $candidates {\n\
if {[string first $name $x] == 0} {\n\
lappend cmds $x\n\
}\n\
}\n\
if {[llength $cmds] == 1} {\n\
return [uplevel 1 [lreplace $args 0 0 [lindex $cmds 0]]]\n\
}\n\
if {[llength $cmds]} {\n\
return -code error \"ambiguous command name \\\"$name\\\": [lsort $cmds]\"\n\
}\n\
}\n\
return -code error \"invalid command name \\\"$name\\\"\"\n\
}\n\
\n\
\n\
proc auto_load {cmd {namespace {}}} {\n\
global auto_index auto_oldpath auto_path\n\
\n\
if {$namespace eq \"\"} {\n\
set namespace [uplevel 1 [list ::namespace current]]\n\
}\n\
set nameList [auto_qualify $cmd $namespace]\n\
lappend nameList $cmd\n\
foreach name $nameList {\n\
if {[info exists auto_index($name)]} {\n\
namespace eval :: $auto_index($name)\n\
if {[namespace which -command $name] ne \"\"} {\n\
return 1\n\
}\n\
}\n\
}\n\
if {![info exists auto_path]} {\n\
return 0\n\
}\n\
\n\
if {![auto_load_index]} {\n\
return 0\n\
}\n\
foreach name $nameList {\n\
if {[info exists auto_index($name)]} {\n\
namespace eval :: $auto_index($name)\n\
if {[namespace which -command $name] ne \"\"} {\n\
return 1\n\
}\n\
}\n\
}\n\
return 0\n\
}\n\
\n\
\n\
proc auto_load_index {} {\n\
global auto_index auto_oldpath auto_path errorInfo errorCode\n\
\n\
if {[info exists auto_oldpath] && $auto_oldpath eq $auto_path} {\n\
return 0\n\
}\n\
set auto_oldpath $auto_path\n\
\n\
\n\
set issafe [interp issafe]\n\
for {set i [expr {[llength $auto_path] - 1}]} {$i >= 0} {incr i -1} {\n\
set dir [lindex $auto_path $i]\n\
set f \"\"\n\
if {$issafe} {\n\
catch {source [file join $dir tclIndex]}\n\
} elseif {[catch {set f [open [file join $dir tclIndex]]}]} {\n\
continue\n\
} else {\n\
set error [catch {\n\
set id [gets $f]\n\
if {$id eq \"# Tcl autoload index file, version 2.0\"} {\n\
eval [read $f]\n\
} elseif {$id eq \"# Tcl autoload index file: each line identifies a Tcl\"} {\n\
while {[gets $f line] >= 0} {\n\
if {[string index $line 0] eq \"#\" \n\
|| ([llength $line] != 2)} {\n\
continue\n\
}\n\
set name [lindex $line 0]\n\
set auto_index($name) \\\n\
\"source [file join $dir [lindex $line 1]]\"\n\
}\n\
} else {\n\
error \"[file join $dir tclIndex] isn't a proper Tcl index file\"\n\
}\n\
} msg]\n\
if {$f ne \"\"} {\n\
close $f\n\
}\n\
if {$error} {\n\
error $msg $errorInfo $errorCode\n\
}\n\
}\n\
}\n\
return 1\n\
}\n\
\n\
\n\
proc auto_qualify {cmd namespace} {\n\
\n\
set n [regsub -all {::+} $cmd :: cmd]\n\
\n\
\n\
\n\
if {[string match ::* $cmd]} {\n\
if {$n > 1} {\n\
return [list $cmd]\n\
} else {\n\
return [list [string range $cmd 2 end]]\n\
}\n\
}\n\
\n\
\n\
if {$n == 0} {\n\
if {$namespace eq \"::\"} {\n\
return [list $cmd]\n\
} else {\n\
return [list ${namespace}::$cmd $cmd]\n\
}\n\
} elseif {$namespace eq \"::\"} {\n\
return [list ::$cmd]\n\
} else {\n\
return [list ${namespace}::$cmd ::$cmd]\n\
}\n\
}\n\
\n\
\n\
proc auto_import {pattern} {\n\
global auto_index\n\
\n\
\n\
if {![string match *::* $pattern]} {\n\
return\n\
}\n\
\n\
set ns [uplevel 1 [list ::namespace current]]\n\
set patternList [auto_qualify $pattern $ns]\n\
\n\
auto_load_index\n\
\n\
foreach pattern $patternList {\n\
foreach name [array names auto_index $pattern] {\n\
if {([namespace which -command $name] eq \"\")\n\
&& ([namespace qualifiers $pattern] eq [namespace qualifiers $name])} {\n\
namespace eval :: $auto_index($name)\n\
}\n\
}\n\
}\n\
}\n\
\n\
\n\
if {$tcl_platform(platform) eq \"windows\"} {\n\
proc auto_execok name {\n\
global auto_execs env tcl_platform\n\
\n\
if {[info exists auto_execs($name)]} {\n\
return $auto_execs($name)\n\
}\n\
set auto_execs($name) \"\"\n\
\n\
set shellBuiltins [list cls copy date del erase dir echo mkdir \\\n\
md rename ren rmdir rd time type ver vol]\n\
if {$tcl_platform(os) eq \"Windows NT\"} {\n\
lappend shellBuiltins \"start\"\n\
}\n\
if {[info exists env(PATHEXT)]} {\n\
set execExtensions [split \";$env(PATHEXT)\" \";\"]\n\
} else {\n\
set execExtensions [list {} .com .exe .bat]\n\
}\n\
\n\
if {[lsearch -exact $shellBuiltins $name] != -1} {\n\
set cmd $env(COMSPEC)\n\
if {[file exists $cmd]} {\n\
set cmd [file attributes $cmd -shortname]\n\
}\n\
return [set auto_execs($name) [list $cmd /c $name]]\n\
}\n\
\n\
if {[llength [file split $name]] != 1} {\n\
foreach ext $execExtensions {\n\
set file ${name}${ext}\n\
if {[file exists $file] && ![file isdirectory $file]} {\n\
return [set auto_execs($name) [list $file]]\n\
}\n\
}\n\
return \"\"\n\
}\n\
\n\
set path \"[file dirname [info nameof]];.;\"\n\
if {[info exists env(WINDIR)]} {\n\
set windir $env(WINDIR) \n\
}\n\
if {[info exists windir]} {\n\
if {$tcl_platform(os) eq \"Windows NT\"} {\n\
append path \"$windir/system32;\"\n\
}\n\
append path \"$windir/system;$windir;\"\n\
}\n\
\n\
foreach var {PATH Path path} {\n\
if {[info exists env($var)]} {\n\
append path \";$env($var)\"\n\
}\n\
}\n\
\n\
foreach dir [split $path {;}] {\n\
if {[info exists checked($dir)] || $dir eq {}} { continue }\n\
set checked($dir) {}\n\
foreach ext $execExtensions {\n\
set file [file join $dir ${name}${ext}]\n\
if {[file exists $file] && ![file isdirectory $file]} {\n\
return [set auto_execs($name) [list $file]]\n\
}\n\
}\n\
}\n\
return \"\"\n\
}\n\
\n\
} else {\n\
proc auto_execok name {\n\
global auto_execs env\n\
\n\
if {[info exists auto_execs($name)]} {\n\
return $auto_execs($name)\n\
}\n\
set auto_execs($name) \"\"\n\
if {[llength [file split $name]] != 1} {\n\
if {[file executable $name] && ![file isdirectory $name]} {\n\
set auto_execs($name) [list $name]\n\
}\n\
return $auto_execs($name)\n\
}\n\
foreach dir [split $env(PATH) :] {\n\
if {$dir eq \"\"} {\n\
set dir .\n\
}\n\
set file [file join $dir $name]\n\
if {[file executable $file] && ![file isdirectory $file]} {\n\
set auto_execs($name) [list $file]\n\
return $auto_execs($name)\n\
}\n\
}\n\
return \"\"\n\
}\n\
\n\
}\n\
\n\
proc tcl::CopyDirectory {action src dest} {\n\
set nsrc [file normalize $src]\n\
set ndest [file normalize $dest]\n\
if {$action eq \"renaming\"} {\n\
if {[lsearch -exact [file volumes] $nsrc] != -1} {\n\
return -code error \"error $action \\\"$src\\\" to\\\n\
\\\"$dest\\\": trying to rename a volume or move a directory\\\n\
into itself\"\n\
}\n\
}\n\
if {[file exists $dest]} {\n\
if {$nsrc eq $ndest} {\n\
return -code error \"error $action \\\"$src\\\" to\\\n\
\\\"$dest\\\": trying to rename a volume or move a directory\\\n\
into itself\"\n\
}\n\
if {$action eq \"copying\"} {\n\
return -code error \"error $action \\\"$src\\\" to\\\n\
\\\"$dest\\\": file already exists\"\n\
} else {\n\
set existing [glob -nocomplain -directory $dest * .*]\n\
eval [linsert \\\n\
[glob -nocomplain -directory $dest -type hidden * .*] 0 \\\n\
lappend existing]\n\
foreach s $existing {\n\
if {([file tail $s] ne \".\") && ([file tail $s] ne \"..\")} {\n\
return -code error \"error $action \\\"$src\\\" to\\\n\
\\\"$dest\\\": file already exists\"\n\
}\n\
}\n\
}\n\
} else {\n\
if {[string first $nsrc $ndest] != -1} {\n\
set srclen [expr {[llength [file split $nsrc]] -1}]\n\
set ndest [lindex [file split $ndest] $srclen]\n\
if {$ndest eq [file tail $nsrc]} {\n\
return -code error \"error $action \\\"$src\\\" to\\\n\
\\\"$dest\\\": trying to rename a volume or move a directory\\\n\
into itself\"\n\
}\n\
}\n\
file mkdir $dest\n\
}\n\
set filelist [concat [glob -nocomplain -directory $src *] \\\n\
[glob -nocomplain -directory $src -types hidden *]]\n\
\n\
foreach s [lsort -unique $filelist] {\n\
if {([file tail $s] ne \".\") && ([file tail $s] ne \"..\")} {\n\
file copy $s [file join $dest [file tail $s]]\n\
}\n\
}\n\
return\n\
}\n\
\n\
\n\
namespace eval tcl {\n\
variable history\n\
if {![info exists history]} {\n\
array set history {\n\
nextid	0\n\
keep	20\n\
oldest	-20\n\
}\n\
}\n\
}\n\
\n\
\n\
proc history {args} {\n\
set len [llength $args]\n\
if {$len == 0} {\n\
return [tcl::HistInfo]\n\
}\n\
set key [lindex $args 0]\n\
set options \"add, change, clear, event, info, keep, nextid, or redo\"\n\
switch -glob -- $key {\n\
a* { # history add\n\
\n\
if {$len > 3} {\n\
return -code error \"wrong # args: should be \\\"history add event ?exec?\\\"\"\n\
}\n\
if {![string match $key* add]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
if {$len == 3} {\n\
set arg [lindex $args 2]\n\
if {! ([string match e* $arg] && [string match $arg* exec])} {\n\
return -code error \"bad argument \\\"$arg\\\": should be \\\"exec\\\"\"\n\
}\n\
}\n\
return [tcl::HistAdd [lindex $args 1] [lindex $args 2]]\n\
}\n\
ch* { # history change\n\
\n\
if {($len > 3) || ($len < 2)} {\n\
return -code error \"wrong # args: should be \\\"history change newValue ?event?\\\"\"\n\
}\n\
if {![string match $key* change]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
if {$len == 2} {\n\
set event 0\n\
} else {\n\
set event [lindex $args 2]\n\
}\n\
\n\
return [tcl::HistChange [lindex $args 1] $event]\n\
}\n\
cl* { # history clear\n\
\n\
if {($len > 1)} {\n\
return -code error \"wrong # args: should be \\\"history clear\\\"\"\n\
}\n\
if {![string match $key* clear]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
return [tcl::HistClear]\n\
}\n\
e* { # history event\n\
\n\
if {$len > 2} {\n\
return -code error \"wrong # args: should be \\\"history event ?event?\\\"\"\n\
}\n\
if {![string match $key* event]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
if {$len == 1} {\n\
set event -1\n\
} else {\n\
set event [lindex $args 1]\n\
}\n\
return [tcl::HistEvent $event]\n\
}\n\
i* { # history info\n\
\n\
if {$len > 2} {\n\
return -code error \"wrong # args: should be \\\"history info ?count?\\\"\"\n\
}\n\
if {![string match $key* info]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
return [tcl::HistInfo [lindex $args 1]]\n\
}\n\
k* { # history keep\n\
\n\
if {$len > 2} {\n\
return -code error \"wrong # args: should be \\\"history keep ?count?\\\"\"\n\
}\n\
if {$len == 1} {\n\
return [tcl::HistKeep]\n\
} else {\n\
set limit [lindex $args 1]\n\
if {[catch {expr {~$limit}}] || ($limit < 0)} {\n\
return -code error \"illegal keep count \\\"$limit\\\"\"\n\
}\n\
return [tcl::HistKeep $limit]\n\
}\n\
}\n\
n* { # history nextid\n\
\n\
if {$len > 1} {\n\
return -code error \"wrong # args: should be \\\"history nextid\\\"\"\n\
}\n\
if {![string match $key* nextid]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
return [expr {$tcl::history(nextid) + 1}]\n\
}\n\
r* { # history redo\n\
\n\
if {$len > 2} {\n\
return -code error \"wrong # args: should be \\\"history redo ?event?\\\"\"\n\
}\n\
if {![string match $key* redo]} {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
return [tcl::HistRedo [lindex $args 1]]\n\
}\n\
default {\n\
return -code error \"bad option \\\"$key\\\": must be $options\"\n\
}\n\
}\n\
}\n\
\n\
\n\
proc tcl::HistAdd {command {exec {}}} {\n\
variable history\n\
\n\
if {[string trim $command] eq \"\"} {\n\
return \"\"\n\
}\n\
\n\
set i [incr history(nextid)]\n\
set history($i) $command\n\
set j [incr history(oldest)]\n\
unset -nocomplain history($j)\n\
if {[string match e* $exec]} {\n\
return [uplevel #0 $command]\n\
} else {\n\
return {}\n\
}\n\
}\n\
\n\
\n\
proc tcl::HistKeep {{limit {}}} {\n\
variable history\n\
if {$limit eq \"\"} {\n\
return $history(keep)\n\
} else {\n\
set oldold $history(oldest)\n\
set history(oldest) [expr {$history(nextid) - $limit}]\n\
for {} {$oldold <= $history(oldest)} {incr oldold} {\n\
unset -nocomplain history($oldold)\n\
}\n\
set history(keep) $limit\n\
}\n\
}\n\
\n\
\n\
proc tcl::HistClear {} {\n\
variable history\n\
set keep $history(keep)\n\
unset history\n\
array set history [list \\\n\
nextid	0	\\\n\
keep	$keep	\\\n\
oldest	-$keep	\\\n\
]\n\
}\n\
\n\
\n\
proc tcl::HistInfo {{num {}}} {\n\
variable history\n\
if {$num eq \"\"} {\n\
set num [expr {$history(keep) + 1}]\n\
}\n\
set result {}\n\
set newline \"\"\n\
for {set i [expr {$history(nextid) - $num + 1}]} \\\n\
{$i <= $history(nextid)} {incr i} {\n\
if {![info exists history($i)]} {\n\
continue\n\
}\n\
set cmd [string map [list \\n \\n\\t] [string trimright $history($i) \\ \\n]]\n\
append result $newline[format \"%6d  %s\" $i $cmd]\n\
set newline \\n\n\
}\n\
return $result\n\
}\n\
\n\
\n\
proc tcl::HistRedo {{event -1}} {\n\
variable history\n\
if {$event eq \"\"} {\n\
set event -1\n\
}\n\
set i [HistIndex $event]\n\
if {$i == $history(nextid)} {\n\
return -code error \"cannot redo the current event\"\n\
}\n\
set cmd $history($i)\n\
HistChange $cmd 0\n\
uplevel #0 $cmd\n\
}\n\
\n\
\n\
proc tcl::HistIndex {event} {\n\
variable history\n\
if {[catch {expr {~$event}}]} {\n\
for {set i [expr {$history(nextid)-1}]} {[info exists history($i)]} \\\n\
{incr i -1} {\n\
if {[string match $event* $history($i)]} {\n\
return $i;\n\
}\n\
if {[string match $event $history($i)]} {\n\
return $i;\n\
}\n\
}\n\
return -code error \"no event matches \\\"$event\\\"\"\n\
} elseif {$event <= 0} {\n\
set i [expr {$history(nextid) + $event}]\n\
} else {\n\
set i $event\n\
}\n\
if {$i <= $history(oldest)} {\n\
return -code error \"event \\\"$event\\\" is too far in the past\"\n\
}\n\
if {$i > $history(nextid)} {\n\
return -code error \"event \\\"$event\\\" hasn't occured yet\"\n\
}\n\
return $i\n\
}\n\
\n\
\n\
proc tcl::HistEvent {event} {\n\
variable history\n\
set i [HistIndex $event]\n\
if {[info exists history($i)]} {\n\
return [string trimright $history($i) \\ \\n]\n\
} else {\n\
return \"\";\n\
}\n\
}\n\
\n\
\n\
proc tcl::HistChange {cmd {event 0}} {\n\
variable history\n\
set i [HistIndex $event]\n\
set history($i) $cmd\n\
}\n\
\n\
\n\
if {$::tcl_platform(platform) eq \"windows\"} {\n\
set tcl_wordchars \"\\\\S\"\n\
set tcl_nonwordchars \"\\\\s\"\n\
} else {\n\
set tcl_wordchars \"\\\\w\"\n\
set tcl_nonwordchars \"\\\\W\"\n\
}\n\
\n\
\n\
proc tcl_wordBreakAfter {str start} {\n\
global tcl_nonwordchars tcl_wordchars\n\
set str [string range $str $start end]\n\
if {[regexp -indices \"$tcl_wordchars$tcl_nonwordchars|$tcl_nonwordchars$tcl_wordchars\" $str result]} {\n\
return [expr {[lindex $result 1] + $start}]\n\
}\n\
return -1\n\
}\n\
\n\
\n\
proc tcl_wordBreakBefore {str start} {\n\
global tcl_nonwordchars tcl_wordchars\n\
if {$start eq \"end\"} {\n\
set start [string length $str]\n\
}\n\
if {[regexp -indices \"^.*($tcl_wordchars$tcl_nonwordchars|$tcl_nonwordchars$tcl_wordchars)\" [string range $str 0 $start] result]} {\n\
return [lindex $result 1]\n\
}\n\
return -1\n\
}\n\
\n\
\n\
proc tcl_endOfWord {str start} {\n\
global tcl_nonwordchars tcl_wordchars\n\
if {[regexp -indices \"$tcl_nonwordchars*$tcl_wordchars+$tcl_nonwordchars\" \\\n\
[string range $str $start end] result]} {\n\
return [expr {[lindex $result 1] + $start}]\n\
}\n\
return -1\n\
}\n\
\n\
\n\
proc tcl_startOfNextWord {str start} {\n\
global tcl_nonwordchars tcl_wordchars\n\
if {[regexp -indices \"$tcl_wordchars*$tcl_nonwordchars+$tcl_wordchars\" \\\n\
[string range $str $start end] result]} {\n\
return [expr {[lindex $result 1] + $start}]\n\
}\n\
return -1\n\
}\n\
\n\
\n\
proc tcl_startOfPreviousWord {str start} {\n\
global tcl_nonwordchars tcl_wordchars\n\
if {$start eq \"end\"} {\n\
set start [string length $str]\n\
}\n\
if {[regexp -indices \\\n\
\"$tcl_nonwordchars*($tcl_wordchars+)$tcl_nonwordchars*\\$\" \\\n\
[string range $str 0 [expr {$start - 1}]] result word]} {\n\
return [lindex $word 0]\n\
}\n\
return -1\n\
}\n\
\n\
package provide http 1.0\n\
\n\
array set http {\n\
-accept */*\n\
-proxyhost {}\n\
-proxyport {}\n\
-useragent {Tcl http client package 1.0}\n\
-proxyfilter httpProxyRequired\n\
}\n\
proc http_config {args} {\n\
global http\n\
set options [lsort [array names http -*]]\n\
set usage [join $options \", \"]\n\
if {[llength $args] == 0} {\n\
set result {}\n\
foreach name $options {\n\
lappend result $name $http($name)\n\
}\n\
return $result\n\
}\n\
regsub -all -- - $options {} options\n\
set pat ^-([join $options |])$\n\
if {[llength $args] == 1} {\n\
set flag [lindex $args 0]\n\
if {[regexp -- $pat $flag]} {\n\
return $http($flag)\n\
} else {\n\
return -code error \"Unknown option $flag, must be: $usage\"\n\
}\n\
} else {\n\
foreach {flag value} $args {\n\
if {[regexp -- $pat $flag]} {\n\
set http($flag) $value\n\
} else {\n\
return -code error \"Unknown option $flag, must be: $usage\"\n\
}\n\
}\n\
}\n\
}\n\
\n\
proc httpFinish { token {errormsg \"\"} } {\n\
upvar #0 $token state\n\
global errorInfo errorCode\n\
if {[string length $errormsg] != 0} {\n\
set state(error) [list $errormsg $errorInfo $errorCode]\n\
set state(status) error\n\
}\n\
catch {close $state(sock)}\n\
catch {after cancel $state(after)}\n\
if {[info exists state(-command)]} {\n\
if {[catch {eval $state(-command) {$token}} err]} {\n\
if {[string length $errormsg] == 0} {\n\
set state(error) [list $err $errorInfo $errorCode]\n\
set state(status) error\n\
}\n\
}\n\
unset state(-command)\n\
}\n\
}\n\
proc http_reset { token {why reset} } {\n\
upvar #0 $token state\n\
set state(status) $why\n\
catch {fileevent $state(sock) readable {}}\n\
httpFinish $token\n\
if {[info exists state(error)]} {\n\
set errorlist $state(error)\n\
unset state(error)\n\
eval error $errorlist\n\
}\n\
}\n\
proc http_get { url args } {\n\
global http\n\
if {![info exists http(uid)]} {\n\
set http(uid) 0\n\
}\n\
set token http#[incr http(uid)]\n\
upvar #0 $token state\n\
http_reset $token\n\
array set state {\n\
-blocksize 	8192\n\
-validate 	0\n\
-headers 	{}\n\
-timeout 	0\n\
state		header\n\
meta		{}\n\
currentsize	0\n\
totalsize	0\n\
type            text/html\n\
body            {}\n\
status		\"\"\n\
}\n\
set options {-blocksize -channel -command -handler -headers \\\n\
-progress -query -validate -timeout}\n\
set usage [join $options \", \"]\n\
regsub -all -- - $options {} options\n\
set pat ^-([join $options |])$\n\
foreach {flag value} $args {\n\
if {[regexp $pat $flag]} {\n\
if {[info exists state($flag)] && \\\n\
[regexp {^[0-9]+$} $state($flag)] && \\\n\
![regexp {^[0-9]+$} $value]} {\n\
return -code error \"Bad value for $flag ($value), must be integer\"\n\
}\n\
set state($flag) $value\n\
} else {\n\
return -code error \"Unknown option $flag, can be: $usage\"\n\
}\n\
}\n\
if {! [regexp -nocase {^(http://)?([^/:]+)(:([0-9]+))?(/.*)?$} $url \\\n\
x proto host y port srvurl]} {\n\
error \"Unsupported URL: $url\"\n\
}\n\
if {[string length $port] == 0} {\n\
set port 80\n\
}\n\
if {[string length $srvurl] == 0} {\n\
set srvurl /\n\
}\n\
if {[string length $proto] == 0} {\n\
set url http://$url\n\
}\n\
set state(url) $url\n\
if {![catch {$http(-proxyfilter) $host} proxy]} {\n\
set phost [lindex $proxy 0]\n\
set pport [lindex $proxy 1]\n\
}\n\
if {$state(-timeout) > 0} {\n\
set state(after) [after $state(-timeout) [list http_reset $token timeout]]\n\
}\n\
if {[info exists phost] && [string length $phost]} {\n\
set srvurl $url\n\
set s [socket $phost $pport]\n\
} else {\n\
set s [socket $host $port]\n\
}\n\
set state(sock) $s\n\
\n\
\n\
fconfigure $s -translation {auto crlf} -buffersize $state(-blocksize)\n\
\n\
\n\
catch {fconfigure $s -blocking off}\n\
set len 0\n\
set how GET\n\
if {[info exists state(-query)]} {\n\
set len [string length $state(-query)]\n\
if {$len > 0} {\n\
set how POST\n\
}\n\
} elseif {$state(-validate)} {\n\
set how HEAD\n\
}\n\
puts $s \"$how $srvurl HTTP/1.0\"\n\
puts $s \"Accept: $http(-accept)\"\n\
puts $s \"Host: $host\"\n\
puts $s \"User-Agent: $http(-useragent)\"\n\
foreach {key value} $state(-headers) {\n\
regsub -all \\[\\n\\r\\]  $value {} value\n\
set key [string trim $key]\n\
if {[string length $key]} {\n\
puts $s \"$key: $value\"\n\
}\n\
}\n\
if {$len > 0} {\n\
puts $s \"Content-Length: $len\"\n\
puts $s \"Content-Type: application/x-www-form-urlencoded\"\n\
puts $s \"\"\n\
fconfigure $s -translation {auto binary}\n\
puts -nonewline $s $state(-query)\n\
} else {\n\
puts $s \"\"\n\
}\n\
flush $s\n\
fileevent $s readable [list httpEvent $token]\n\
if {! [info exists state(-command)]} {\n\
http_wait $token\n\
}\n\
return $token\n\
}\n\
proc http_data {token} {\n\
upvar #0 $token state\n\
return $state(body)\n\
}\n\
proc http_status {token} {\n\
upvar #0 $token state\n\
return $state(status)\n\
}\n\
proc http_code {token} {\n\
upvar #0 $token state\n\
return $state(http)\n\
}\n\
proc http_size {token} {\n\
upvar #0 $token state\n\
return $state(currentsize)\n\
}\n\
\n\
proc httpEvent {token} {\n\
upvar #0 $token state\n\
set s $state(sock)\n\
\n\
if {[eof $s]} {\n\
httpEof $token\n\
return\n\
}\n\
if {$state(state) == \"header\"} {\n\
set n [gets $s line]\n\
if {$n == 0} {\n\
set state(state) body\n\
if {![regexp -nocase ^text $state(type)]} {\n\
fconfigure $s -translation binary\n\
if {[info exists state(-channel)]} {\n\
fconfigure $state(-channel) -translation binary\n\
}\n\
}\n\
if {[info exists state(-channel)] &&\n\
![info exists state(-handler)]} {\n\
fileevent $s readable {}\n\
httpCopyStart $s $token\n\
}\n\
} elseif {$n > 0} {\n\
if {[regexp -nocase {^content-type:(.+)$} $line x type]} {\n\
set state(type) [string trim $type]\n\
}\n\
if {[regexp -nocase {^content-length:(.+)$} $line x length]} {\n\
set state(totalsize) [string trim $length]\n\
}\n\
if {[regexp -nocase {^([^:]+):(.+)$} $line x key value]} {\n\
lappend state(meta) $key $value\n\
} elseif {[regexp ^HTTP $line]} {\n\
set state(http) $line\n\
}\n\
}\n\
} else {\n\
if {[catch {\n\
if {[info exists state(-handler)]} {\n\
set n [eval $state(-handler) {$s $token}]\n\
} else {\n\
set block [read $s $state(-blocksize)]\n\
set n [string length $block]\n\
if {$n >= 0} {\n\
append state(body) $block\n\
}\n\
}\n\
if {$n >= 0} {\n\
incr state(currentsize) $n\n\
}\n\
} err]} {\n\
httpFinish $token $err\n\
} else {\n\
if {[info exists state(-progress)]} {\n\
eval $state(-progress) {$token $state(totalsize) $state(currentsize)}\n\
}\n\
}\n\
}\n\
}\n\
proc httpCopyStart {s token} {\n\
upvar #0 $token state\n\
if {[catch {\n\
fcopy $s $state(-channel) -size $state(-blocksize) -command \\\n\
[list httpCopyDone $token]\n\
} err]} {\n\
httpFinish $token $err\n\
}\n\
}\n\
proc httpCopyDone {token count {error {}}} {\n\
upvar #0 $token state\n\
set s $state(sock)\n\
incr state(currentsize) $count\n\
if {[info exists state(-progress)]} {\n\
eval $state(-progress) {$token $state(totalsize) $state(currentsize)}\n\
}\n\
if {([string length $error] != 0)} {\n\
httpFinish $token $error\n\
} elseif {[eof $s]} {\n\
httpEof $token\n\
} else {\n\
httpCopyStart $s $token\n\
}\n\
}\n\
proc httpEof {token} {\n\
upvar #0 $token state\n\
if {$state(state) == \"header\"} {\n\
set state(status) eof\n\
} else {\n\
set state(status) ok\n\
}\n\
set state(state) eof\n\
httpFinish $token\n\
}\n\
proc http_wait {token} {\n\
upvar #0 $token state\n\
if {![info exists state(status)] || [string length $state(status)] == 0} {\n\
vwait $token\\(status)\n\
}\n\
if {[info exists state(error)]} {\n\
set errorlist $state(error)\n\
unset state(error)\n\
eval error $errorlist\n\
}\n\
return $state(status)\n\
}\n\
\n\
\n\
proc http_formatQuery {args} {\n\
set result \"\"\n\
set sep \"\"\n\
foreach i $args {\n\
append result  $sep [httpMapReply $i]\n\
if {$sep != \"=\"} {\n\
set sep =\n\
} else {\n\
set sep &\n\
}\n\
}\n\
return $result\n\
}\n\
\n\
\n\
proc httpMapReply {string} {\n\
global httpFormMap\n\
set alphanumeric	a-zA-Z0-9\n\
if {![info exists httpFormMap]} {\n\
\n\
for {set i 1} {$i <= 256} {incr i} {\n\
set c [format %c $i]\n\
if {![string match \\[$alphanumeric\\] $c]} {\n\
set httpFormMap($c) %[format %.2x $i]\n\
}\n\
}\n\
array set httpFormMap {\n\
\" \" +   \\n %0d%0a\n\
}\n\
}\n\
regsub -all \\[^$alphanumeric\\] $string {$httpFormMap(&)} string\n\
regsub -all \\n $string {\\\\n} string\n\
regsub -all \\t $string {\\\\t} string\n\
regsub -all {[][{})\\\\]\\)} $string {\\\\&} string\n\
return [subst $string]\n\
}\n\
\n\
proc httpProxyRequired {host} {\n\
global http\n\
if {[info exists http(-proxyhost)] && [string length $http(-proxyhost)]} {\n\
if {![info exists http(-proxyport)] || ![string length $http(-proxyport)]} {\n\
set http(-proxyport) 8080\n\
}\n\
return [list $http(-proxyhost) $http(-proxyport)]\n\
} else {\n\
return {}\n\
}\n\
}\n\
";
#include "tclcl.h"
EmbeddedTcl et_tcl(code);
