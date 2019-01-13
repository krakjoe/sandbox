sandbox
=======

[![Build Status](https://travis-ci.org/krakjoe/sandbox.svg?branch=develop)](https://travis-ci.org/krakjoe/sandbox)
[![Build status](https://ci.appveyor.com/api/projects/status/j78jcoc3vg3ptg54?svg=true)](https://ci.appveyor.com/project/krakjoe/sandbox)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/sandbox/badge.svg?branch=develop)](https://coveralls.io/github/krakjoe/sandbox)

A sandbox is an isolated environment (a thread in our case); Things may go very badly wrong in the sandbox environment and not effect the environment that created it. This means that we must try very hard to limit the influence each environment has on the other. So the prototype and instructions of entry point ```Closures``` are verified to ensure they will not reduce or break isolation.

In practice this means entry point closures must not:

  * accept or return by reference
  * accept or return objects
  * execute a limited set of instructions

Instructions prohibited directly in the sandbox are:

  * declare (anonymous) function
  * declare (anonymous) class
  * lexical scope access
  * yield

No instructions are prohibited in the files which the sandbox may include, but allowing these instructions directly in the code which the sandbox executes at entry would break the isolation of the sandbox such that we couldn't be sure the system would remain stable.

With these restrictions in place, we can be sure that a sandbox may do anything up to but excluding making PHP segfault, and not effect the environment that created it.

Requirements
============

  * PHP 7.1+
  * ZTS
  * <pthread.h>

API
===

```php
final class sandbox\Runtime {
	/**
	* Shall construct a new sandbox thread
        * @param array optional ini configuration
	* @throws \sandbox\Exception if the sandbox could not be created
	*/
	public function __construct(array $ini = []);
	
	/**
	* Shall enter into the thread at the given entry point
	* @param entry point for sandbox
	* @param argv for closure
	* @throws \sandbox\Exception if $closure or $argv are not valid
	* @throws \sandbox\Exception if $closure bails
	**/
	public function enter(Closure $closure, array $argv = []) : mixed;

	/*
	* Shall close the sandbox
	* @throws \sandbox\Exception if the sandbox is unusable
	*/
	public function close();
}
```

Configuring the Sandbox
=======================

The configuration array passed to the constructor will be configure the sandbox using INI.

The following options may be an array, or comma separated list:

  * disable_functions
  * disable_classes
  * extension 
  * zend_extension

All other options are passed directly to zend verbatim and set as if set by system configuration file.

Extensions
==========
*PHP isn't really share nothing, it's share as little as possible to get the job done!*

It's possible to load extensions in the sandbox that are not available in the parent runtime, however this comes with a (benign, mostly) quirk.

```php
$sandbox = new \sandbox\Runtime([
	"extension" => [
		"componere"
	]
]);

$sandbox->enter(function(){
	var_dump(new \Componere\Definition("mine"));
});

if (extension_loaded("componere")){
	var_dump(new \Componere\Definition("mine")); # line 13
}
```

The code above will output something like:

```
object(Componere\Definition)#1 (0) {
}

Fatal error: Uncaught Error: Class 'Componere\Definition' not found in %s:13
Stack trace:
#0 {main}
  thrown in %s on line 13
```

The reason for this behaviour is that the extension registry is a truly global symbol, and so the parent context does detect that the extension is loaded, but it is not able to detect that it was never started in the this context and so did not register any classes.

The same is true when the sandboxed code executes ```dl```, which may be disabled with confgiuration in the normal way.
