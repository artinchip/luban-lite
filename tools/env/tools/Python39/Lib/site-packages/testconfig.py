from nose.plugins.base import Plugin
from nose.util import tolist

import os

try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser

import logging
import codecs

log = logging.getLogger(__name__)

warning = "Cannot access the test config because the plugin has not \
been activated.  Did you specify --tc or any other command line option?"

config = {}

def merge_map(original, to_add):
    """ Merges a new map of configuration recursively with an older one """
    for k, v in to_add.items():
        if isinstance(v, dict) and k in original and isinstance(original[k],
                                                                dict):
            merge_map(original[k], v)
        else:
            original[k] = v

def load_yaml(yaml_file, encoding):
    """ Load the passed in yaml configuration file """
    try:
        import yaml
    except (ImportError):
        raise Exception('unable to import YAML package. Can not continue.')
    global config
    parsed_config = yaml.load(codecs.open(yaml_file, 'r', encoding).read())
    merge_map(config, parsed_config)


def load_ini(ini_file, encoding):
    """ Parse and collapse a ConfigParser-Style ini file into a two-level
    config structure. """

    global config
    tmpconfig = ConfigParser.ConfigParser()
    with codecs.open(ini_file, 'r', encoding) as f:
        tmpconfig.readfp(f)

    parsed_config = {}
    for section in tmpconfig.sections():
        parsed_config[section] = {}
        for option in tmpconfig.options(section):
            parsed_config[section][option] = tmpconfig.get(section, option)
    merge_map(config, parsed_config)

def load_python(py_file, encoding):
    """ This will exec the defined python file into the config variable -
    the implicit assumption is that the python is safe, well formed and will
    not do anything bad. This is also dangerous. """
    exec(codecs.open(py_file, 'r', encoding).read())


def load_json(json_file, encoding):
    """ This will use the json module to to read in the config json file.
    """
    import json
    global config
    with codecs.open(json_file, 'r', encoding=encoding) as handle:
        parsed_config = json.load(handle)
    merge_map(config, parsed_config)


class TestConfig(Plugin):

    enabled = False
    name = "test_config"
    score = 1

    env_opt = "NOSE_TEST_CONFIG_FILE"
    format = "ini"
    encoding = 'utf-8'
    valid_loaders = { 'yaml' : load_yaml, 'ini' : load_ini,
                      'python' : load_python, 'json': load_json }

    def options(self, parser, env=os.environ):
        """ Define the command line options for the plugin. """
        parser.add_option(
            "--tc-file", action="append",
            default=[env.get(self.env_opt)],
            dest="testconfig",
            help="Configuration file to parse and pass to tests"
                 " [NOSE_TEST_CONFIG_FILE]")
        parser.add_option(
            "--tc-file-encoding", action="store",
            default=env.get('NOSE_TEST_CONFIG_FILE_ENCODING') or self.encoding,
            dest="testconfigencoding",
            help="Test config file encoding, default is utf-8"
                 " [NOSE_TEST_CONFIG_FILE_ENCODING]")
        parser.add_option(
            "--tc-format", action="store",
            default=env.get('NOSE_TEST_CONFIG_FILE_FORMAT') or self.format,
            dest="testconfigformat",
            help="Test config file format, default is configparser ini format"
                 " [NOSE_TEST_CONFIG_FILE_FORMAT]")
        parser.add_option(
            "--tc", action="append",
            dest="overrides",
            default = [],
            help="Option:Value specific overrides.")
        parser.add_option(
            "--tc-exact", action="store_true",
            dest="exact",
            default = False,
            help="Optional: Do not explode periods in override keys to "
                 "individual keys within the config dict, instead treat them"
                 " as config[my.toplevel.key] ala sqlalchemy.url in pylons")

    def configure(self, options, noseconfig):
        """ Call the super and then validate and call the relevant parser for
        the configuration file passed in """
        if not options.testconfig and not options.overrides:
            return
        Plugin.configure(self, options, noseconfig)

        self.config = noseconfig
        if not options.capture:
            self.enabled = False
        if options.testconfigformat:
            self.format = options.testconfigformat
            if self.format not in self.valid_loaders.keys():
                raise Exception('%s is not a valid configuration file format' \
                                                                % self.format)

        # Load the configuration file:
        for configfile in options.testconfig:
            if configfile:
                self.valid_loaders[self.format](configfile,
                                                options.testconfigencoding)

        overrides = tolist(options.overrides) or []
        for override in overrides:
            keys, val = override.split(":", 1)
            if options.exact:
                config[keys] = val
            else:
                # Create all *parent* keys that may not exist in the config
                section = config
                keys = keys.split('.')
                for key in keys[:-1]:
                    if key not in section:
                        section[key] = {}
                    section = section[key]

                # Finally assign the value to the last key
                key = keys[-1]
                section[key] = val


# Use an environment hack to allow people to set a config file to auto-load
# in case they want to put tests they write through pychecker or any other
# syntax thing which does an execute on the file.
if 'NOSE_TESTCONFIG_AUTOLOAD_YAML' in os.environ:
    load_yaml(os.environ['NOSE_TESTCONFIG_AUTOLOAD_YAML'], encoding='utf-8')

if 'NOSE_TESTCONFIG_AUTOLOAD_INI' in os.environ:
    load_ini(os.environ['NOSE_TESTCONFIG_AUTOLOAD_INI'], encoding='utf-8')

if 'NOSE_TESTCONFIG_AUTOLOAD_PYTHON' in os.environ:
    load_python(os.environ['NOSE_TESTCONFIG_AUTOLOAD_PYTHON'], encoding='utf-8')

if 'NOSE_TESTCONFIG_AUTOLOAD_JSON' in os.environ:
    load_json(os.environ['NOSE_TESTCONFIG_AUTOLOAD_JSON'], encoding='utf-8')
