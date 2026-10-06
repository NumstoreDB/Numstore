#!/usr/bin/env node
// Builds the in-memory stub libnumstore, links the addon against it and runs
// the test suite. Use this to test the bindings without a real numstore build.
// (Linux / macOS.)
'use strict';

const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const root = path.join(__dirname, '..');
const out = path.join(root, 'test', 'stub', 'build');
const isMac = process.platform === 'darwin';
const lib = path.join(out, isMac ? 'libnumstore.dylib' : 'libnumstore.so');

fs.mkdirSync(out, { recursive: true });

const run = (cmd, args, env) =>
  execFileSync(cmd, args, { cwd: root, stdio: 'inherit', env: { ...process.env, ...env } });

run(process.env.CC || 'cc', [
  '-shared', '-fPIC', '-std=c11', '-Wall', '-Wextra', '-g',
  ...(isMac ? ['-install_name', '@rpath/libnumstore.dylib'] : []),
  '-I', path.join(root, 'include'),
  path.join(root, 'test', 'stub', 'numstore_stub.c'),
  '-o', lib,
]);

run('npx', ['--yes', 'node-gyp', 'rebuild'], {
  NUMSTORE_INCLUDE_DIR: path.join(root, 'include'),
  NUMSTORE_LIB_DIR: out,
});

run(process.execPath, ['--expose-gc', '--test', path.join(root, 'test', 'numstore.test.js')]);
