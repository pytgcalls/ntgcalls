import { execSync } from 'node:child_process';

function arg(name, fallback) {
  const i = process.argv.indexOf(`--${name}`);
  if (i !== -1 && i + 1 < process.argv.length) return process.argv[i + 1];
  if (fallback !== undefined) return fallback;
  throw new Error(`missing required argument --${name}`);
}

const spec = arg('spec');
const version = arg('version');
const tag = arg('tag');
const dir = arg('dir', '.');

function alreadyPublished() {
  try {
    const out = execSync(`npm view ${spec}@${version} version`, {
      stdio: ['ignore', 'pipe', 'ignore'],
    }).toString().trim();
    return out.length > 0;
  } catch (_) {
    return false;
  }
}

if (alreadyPublished()) {
  console.log(`skip ${spec}@${version} (already published)`);
} else {
  execSync(`npm publish ${dir} --access public --tag ${tag}`, { stdio: 'inherit' });
}
