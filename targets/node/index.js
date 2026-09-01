'use strict';

const { platform, arch } = process;

function load() {
  try {
    return require(`@ntgcalls/${platform}-${arch}`);
  } catch (_) {
    try {
      return require('./build/Release/ntgcalls.node');
    } catch (_) {
      throw new Error(
        `ntgcalls: no prebuilt binary for ${platform}-${arch}. ` +
        `Install the matching @ntgcalls/${platform}-${arch} package ` +
        `or build from source with "npm run build".`
      );
    }
  }
}

module.exports = load();
