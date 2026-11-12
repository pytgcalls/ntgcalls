'use strict';

const { platform, arch } = process;

function libcSuffix() {
  if (platform !== 'linux') return '';
  return process.report.getReport().header.glibcVersionRuntime ? '' : '-musl';
}

const name = `${platform}-${arch}${libcSuffix()}`;

function load() {
  try {
    return require(`@ntgcalls/${name}`);
  } catch (_) {
    try {
      return require('./build/Release/ntgcalls.node');
    } catch (_) {
      throw new Error(
        `ntgcalls: no prebuilt binary for ${name}. ` +
        `Install the matching @ntgcalls/${name} package ` +
        `or build from source with "npm run build".`
      );
    }
  }
}

module.exports = load();
