==========
testconfig
==========

* Project hosting: <http://bitbucket.org/jnoller/nose-testconfig/>

.. contents::

About
------------------

Written by Jesse Noller  
Licensed under the Apache Software License, 2.0

You can install it with ``pip install nose-testconfig``

What It Does
------------

nose-testconfig is a plugin to the nose test framework which provides a
faculty for passing test-specific (or test-run specific) configuration data
to the tests being executed.

Currently configuration files in the following formats are supported:

- YAML (via `PyYAML <http://pypi.python.org/pypi/PyYAML/>`_)
- INI (via `ConfigParser <http://docs.python.org/lib/module-ConfigParser.html>`_)
- Pure Python (via Exec)
- JSON (via `JSON <http://docs.python.org/library/json.html>`_)

The plugin is ``meant`` to be flexible, ergo the support of exec'ing arbitrary
python files as configuration files with no checks. The default format is 
assumed to be ConfigParser ini-style format.

If multiple files are provided, the objects are merged. Later settings will
override earlier ones.

The plugin provides a method of overriding certain parameters from the command 
line (assuming that the main "config" object is a dict) and can easily have 
additional parsers added to it.

A configuration file may not be provided. In this case, the config object is an
emtpy dict. Any command line "overriding" paramters will be added to the dict.

Test Usage
----------

For now (until something better comes along) tests can import the "config" 
singleton from testconfig::

    from testconfig import config

By default, YAML files parse into a nested dictionary, and ConfigParser ini
files are also collapsed into a nested dictionary for foo[bar][baz] style
access. Tests can obviously access configuration data by referencing the 
relevant dictionary keys::

    from testconfig import config
    def test_foo():
        target_server_ip = config['servers']['webapp_ip']

``Warning``: Given this is just a dictionary singleton, tests can easily write
into the configuration. This means that your tests can write into the config
space and possibly alter it. This also means that threaded access into the
configuration can be interesting.

When using pure python configuration - obviously the "sky is the the limit" - 
given that the configuration is loaded via an exec, you could potentially
modify nose, the plugin, etc. However, if you do not export a config{} dict
as part of your python code, you obviously won't be able to import the 
config object from testconfig.

When using YAML-style configuration, you get a lot of the power of pure python
without the danger of unprotected exec() - you can obviously use the pyaml 
python-specific objects and all of the other YAML creamy goodness.

Defining a configuration file
-----------------------------

Simple ConfigParser style::

    [myapp_servers]
    main_server = 10.1.1.1
    secondary_server = 10.1.1.2

So your tests access the config options like this::

    from testconfig import config
    def test_foo():
        main_server = config['myapp_servers']['main_server']

YAML style configuration::
    myapp:
        servers:
            main_server: 10.1.1.1
            secondary_server: 10.1.1.2

And your tests can access it thus::

    from testconfig import config
    def test_foo():
        main_server = config['myapp']['servers']['main_server']

Python configuration file::

    import socket

    global config
    config = {}
    possible_main_servers = ['10.1.1.1', '10.1.1.2']

    for srv in possible_main_servers:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((srv, 80))
        except:
            continue
        s.close()
        config['main_server'] = srv
        break

And lo, the config is thus::

    from testconfig import config
    def test_foo():
        main_server = config['main_server']

If you need to put python code into your configuration, you either need to use
the python-config file faculties, or you need to use the !!python tags within
PyYAML/YAML - raw ini files no longer have any sort of eval magic.

Command line options
--------------------

After it is installed, the plugin adds the following command line flags to 
nosetests::

    --tc-file=TESTCONFIG  Configuration file to parse and pass to tests
                          [NOSE_TEST_CONFIG_FILE]
                          If this is specified multiple times, all files
                          will be parsed. In all formats except python,
                          previous contents are preserved and the configs
                          are merged.

    --tc-format=TESTCONFIGFORMAT  Test config file format, default is
                                  configparser ini format
                                  [NOSE_TEST_CONFIG_FILE_FORMAT]

    --tc=OVERRIDES        Option:Value specific overrides.

    --tc-exact            Optional: Do not explode periods in override keys to
                          individual keys within the config dict, instead treat
                          them as config[my.toplevel.key] ala sqlalchemy.url in
                          pylons.


Passing in an INI configuration file::

    $ nosetests -s --tc-file example_cfg.ini

Passing in a YAML configuration file::

    $ nosetests -s --tc-file example_cfg.yaml --tc-format yaml

Passing in a Python configuration file::

    $ nosetests -s --tc-file example_cfg.py --tc-format python

Passing in multiple INI configuration files::

    $ nosetests -s --tc-file example_cfg.ini --tc-file example_cfg2.ini

Overriding a configuration value on the command line::

    $ nosetests -s --tc-file example_cfg.ini --tc=myvalue.sub:bar

Passing parameters on the command line without specifying a configuration file::

    $ nosetests -s --tc=myvalue.sub2:baz

Overriding multiple key:value pairs::

    $ nosetests -s --tc-file example_cfg.ini --tc=myvalue.sub:bar \
        --tc=myvalue.sub2:baz --tc=myvalue.sub3:bar3


``Warning``: When using the --tc= flag, you can pass it in as many times as
you want to override as many keys/values as needed. The format is in
``parent.child.child = value`` format - the periods are translated into keys
within the config dict, for example::

    myvalue.sub2:baz = config[myvalue][sub2] = baz

You can override the explosion of the periods by passing in the --tc-exact 
argument on the command line.

Special environment variables
-----------------------------

If you have a test which performs an import like this::

    from testconfig import config

Then you know you can not run your test through a tool like pychecker as 
pychecker executes the file you are scanning, an warning is thrown and any use
of the config dict will cause an exception.

To work around this, I've added four environment variable checks which, if 
set will cause a given configuration file to be auto-loaded into the module
and the config dict will be populated. These are::

    NOSE_TESTCONFIG_AUTOLOAD_YAML
    NOSE_TESTCONFIG_AUTOLOAD_INI
    NOSE_TESTCONFIG_AUTOLOAD_PYTHON
    NOSE_TESTCONFIG_AUTOLOAD_JSON

Setting one of these to ``full path`` of the target configuration file in your
environment/editor/etc will make it auto load that configuration file. You can
now run it through pychecker. Much success was had!

For example, I set NOSE_TESTCONFIG_AUTOLOAD_YAML to /Users/jesse/foo.yaml 
within textmate. I can now use pychecker via control-shift-v with much win.

Changes & News
--------------
0.10:
    * support multiple config files

0.9.1:
    * update tox with pypy and py34
    * advertise with classifiers that nose-testconfig is py2 and py3 compatible

0.9:
    * Python 3 compatible
    * fix loading of data files when environment variables
      NOSE_TESTCONFIG_AUTOLOAD_* are specified.
    * added tests

0.8:
    * unicode support for config files (gjednaszewski/dhellmann)
    * colons are allowed in user's arguments, such as --tc url:127.0.0.1:5000 (aconrad)
    * config file is not longer required, --tc option may be provided alone (aconrad)

0.7:
    * Add ability to handle json format.

0.6:
    * Add in checking for 3 different environment variables corresponding to
      the supported config file types. Setting one of these to the full path
      to a given configuration file will force nose-testconfig to autoload that
      file. Handy if you want to run a test which imports the testconfig module
      through something like pychecker (or run it from the command line).

0.5:
    * Fix a bug in the python config file parsing reported by Christopher Hesse

0.4:
    * Per feedback from Kumar and others, the eval()'ing of ini-file values 
      has been removed: allowing arbitrary python in the values was more 
      annoying less standard then was worth it.
    * Added the --tc-exact command line flag, to block the exploding of 
      name.name values into dicts-within-dicts
    * Updated the docs to parse right.

0.3: 
    Fix documentation examples per Kumar's feedback.

0.2:
    Fix pypi packaging issues

0.1:
    Initial release.  May contain bits of glass.

