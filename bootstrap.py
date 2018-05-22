#!/usr/bin/env python2
import argparse
import os
import subprocess
import sys
import errno

DEPOT_TOOLS_URL='https://chromium.googlesource.com/chromium/tools/depot_tools.git'
THIRD_PARTY_PATH = '%s/third_party' % os.path.dirname(os.path.realpath(__file__))
DEPOT_TOOLS_PATH = '%s/depot_tools' % THIRD_PARTY_PATH
REVISION = '6.5.257'

class TargetAlreadyExists(Exception):
  def __init__(self, name):
    super(Exception, self).__init__()
    self.target_name = name

class MissingDependency(Exception):
  def __init__(self, name):
    super(Exception, self).__init__()
    self.dependency_name = name

def print_progress(msg):
  sys.stderr.write(msg + '\n')
  sys.stderr.flush()

def die(msg):
  sys.stderr.write(msg + '\n')
  sys.stderr.flush()
  sys.exit(1)

def prepend_path(path, new_path):
  if len(path) > 0:
    return '%s%s%s' % (new_path, os.pathsep, path)
  else:
    return new_path

def target_exists(target_name):
  return os.path.exists(os.path.join(THIRD_PARTY_PATH, target_name))

def fetch_depot_tools():
  """Download depot_tools to DEPOT_TOOLS_PATH."""
  if target_exists('depot_tools'):
      raise TargetAlreadyExists('depot_tools')
  subprocess.check_call(['git', 'clone', DEPOT_TOOLS_URL, DEPOT_TOOLS_PATH])

def fetch_gclient_based_dependencies():
  """Download a dependencies using gclient from depot_tools.
  depot_tools must be installed in DEPOT_TOOLS_PATH."""
  new_environ = os.environ.copy()
  new_environ['PATH'] = \
      prepend_path(os.environ.get('PATH', ''), DEPOT_TOOLS_PATH)

  # Update gclient.
  subprocess.check_call(['gclient'], env=new_environ, cwd=THIRD_PARTY_PATH)

  # Fetch v8.
  subprocess.check_call(['fetch', 'v8'], env=new_environ, cwd=THIRD_PARTY_PATH)
  v8_dir = os.path.join(THIRD_PARTY_PATH, 'v8')
  subprocess.check_call(['gclient', 'sync', '--revision', REVISION],
      env=new_environ, cwd=v8_dir)

def build_v8():
  """Build v8 (arch: native, in release mode). All dependencies must
  be downloaded in advance."""
  if not target_exists('depot_tools'):
    raise MissingDependency('depot_tools')
  if not target_exists('v8'):
    raise MissingDependency('v8')

  new_environ = os.environ.copy()
  new_environ['PATH'] = \
      prepend_path(os.environ.get('PATH', ''), DEPOT_TOOLS_PATH)

  v8_gen = os.path.join(THIRD_PARTY_PATH, 'v8', 'tools', 'dev', 'v8gen.py')
  v8_dir = os.path.join(THIRD_PARTY_PATH, 'v8')
  print_progress("Generating makefiles ...")
  subprocess.check_call(
      [ v8_gen, 'x64.release', '--'
      , 'v8_use_snapshot=false'
      , 'v8_use_external_startup_data=false'
      , 'v8_enable_i18n_support=true'
      , 'v8_monolithic=true'
      ]
      , env=new_environ, cwd=v8_dir)
  print_progress("Starting build ...")
  subprocess.check_call(
      [ 'ninja', '-C', os.path.join('out.gn', 'x64.release')]
      ,  env=new_environ, cwd=v8_dir)

def fetch_command_handler(args):
  try:
    fetch_depot_tools()
  except TargetAlreadyExists:
    print_progress('Dependency \'depot_tools\' already exists.')
  fetch_gclient_based_dependencies()

def build_command_handler(args):
  build_v8()
  print_progress('SUCCESS')

def main(args):
  arg_parser = argparse.ArgumentParser(
      description='Manage third party dependencies.')

  subparsers = arg_parser.add_subparsers(help='supported commands')
  fetch_parser = subparsers.add_parser('fetch', help='fetch dependencies')
  fetch_parser.set_defaults(fn=fetch_command_handler)
  build_parser = subparsers.add_parser('build', help='build dependencies')
  build_parser.add_argument('deps', metavar='DEP', nargs='*', default='all',
      choices=['v8', 'all'],
      help='a list of dependencies to build (possible choices: v8, all)')
  build_parser.set_defaults(fn=build_command_handler)

  parsed_args = arg_parser.parse_args(args[1:])
  parsed_args.fn(parsed_args)

if __name__ == '__main__':
  sys.exit(main(sys.argv))
