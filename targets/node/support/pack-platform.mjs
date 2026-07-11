import { mkdirSync, copyFileSync, writeFileSync, readFileSync } from 'node:fs';
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

const binary = resolve(arg('binary'));
const platform = arg('platform', process.platform);
const arch = arg('arch', process.arch);
const version = arg('version', JSON.parse(readFileSync(join(root, 'package.json'), 'utf8')).version);
const outDir = resolve(arg('out', join(root, 'packages')));

const main = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
const pkgDir = join(outDir, `${platform}-${arch}`);
mkdirSync(pkgDir, { recursive: true });

copyFileSync(binary, join(pkgDir, 'ntgcalls.node'));

const pkg = {
  name: `@ntgcalls/${platform}-${arch}`,
  version,
  description: `${main.description} (prebuilt binary for ${platform}-${arch})`,
  author: main.author,
  license: main.license,
  homepage: main.homepage,
  repository: main.repository,
  engines: main.engines,
  os: [platform],
  cpu: [arch],
  main: 'ntgcalls.node',
  files: ['ntgcalls.node'],
};

writeFileSync(join(pkgDir, 'package.json'), `${JSON.stringify(pkg, null, 2)}\n`);

console.log(`packed @ntgcalls/${platform}-${arch}@${version} -> ${pkgDir}`);
