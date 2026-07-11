import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, '..');

function arg(name, fallback) {
  const i = process.argv.indexOf(`--${name}`);
  if (i !== -1 && i + 1 < process.argv.length) return process.argv[i + 1];
  if (fallback !== undefined) return fallback;
  throw new Error(`missing required argument --${name}`);
}

const version = arg('version');
const path = join(root, 'package.json');
const pkg = JSON.parse(readFileSync(path, 'utf8'));

pkg.version = version;
for (const name of Object.keys(pkg.optionalDependencies ?? {})) {
  pkg.optionalDependencies[name] = version;
}

writeFileSync(path, `${JSON.stringify(pkg, null, 2)}\n`);
console.log(`stamped ntgcalls@${version}`);
