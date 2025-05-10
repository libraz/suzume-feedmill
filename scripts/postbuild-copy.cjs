/* eslint-disable @typescript-eslint/no-require-imports */
/**
 * Post-build script to copy files to the correct locations
 */

const fs = require('fs');
const path = require('path');

// Paths
const rootDir = path.resolve(__dirname, '..');
const distDir = path.join(rootDir, 'dist');
const libDir = path.join(rootDir, 'lib');
const binDir = path.join(rootDir, 'bin');

// Ensure directories exist
function ensureDirExists(dir) {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }
}

// Copy file
function copyFile(src, dest) {
  console.log(`Copying ${src} to ${dest}`);
  fs.copyFileSync(src, dest);
}

// Make file executable
function makeExecutable(file) {
  try {
    fs.chmodSync(file, '755');
    console.log(`Made ${file} executable`);
  } catch (err) {
    console.error(`Failed to make ${file} executable: ${err.message}`);
  }
}

// Main function
function main() {
  try {
    // Ensure directories exist
    ensureDirExists(distDir);
    ensureDirExists(path.join(distDir, 'cjs'));
    ensureDirExists(path.join(distDir, 'esm'));
    ensureDirExists(libDir);
    ensureDirExists(binDir);

    // Copy TypeScript declaration files
    copyFile(path.join(rootDir, 'ts', 'index.d.ts'), path.join(distDir, 'index.d.ts'));

    // Create index.js files if they don't exist
    const cjsIndexPath = path.join(distDir, 'cjs', 'index.js');
    const esmIndexPath = path.join(distDir, 'esm', 'index.js');

    if (!fs.existsSync(cjsIndexPath)) {
      console.log(`Creating ${cjsIndexPath}`);
      fs.writeFileSync(
        cjsIndexPath,
        `
/**
 * suzume-feedmill: High-performance corpus preprocessing engine for n-gram and PMI extraction
 *
 * @packageDocumentation
 */

const bindings = require('bindings');

// Load native module
const native = bindings('suzume_feedmill');

/**
 * Normalize text data
 *
 * @param {string|object|Buffer} input Input source (file path, file descriptor, stream, or buffer)
 * @param {string|object|null} output Output destination (file path, file descriptor, stream, or null)
 * @param {object} options Normalization options
 * @returns {Promise<object>} Promise that resolves to normalization result
 */
function normalize(input, output, options) {
  return native.normalize(input, output, options || {});
}

/**
 * Calculate PMI (Pointwise Mutual Information)
 *
 * @param {string|object|Buffer} input Input source (file path, file descriptor, stream, or buffer)
 * @param {string|object|null} output Output destination (file path, file descriptor, stream, or null)
 * @param {object} options PMI calculation options
 * @returns {Promise<object>} Promise that resolves to PMI calculation result
 */
function pmi(input, output, options) {
  return native.pmi(input, output, options || {});
}

// Export functions
module.exports = {
  normalize,
  pmi
};
      `
      );
    }

    if (!fs.existsSync(esmIndexPath)) {
      console.log(`Creating ${esmIndexPath}`);
      fs.writeFileSync(
        esmIndexPath,
        `
/**
 * suzume-feedmill: High-performance corpus preprocessing engine for n-gram and PMI extraction
 * ESM entry point
 *
 * @packageDocumentation
 */

// Import from CJS version
const cjs = require('../cjs/index.js');

/**
 * Normalize text data
 *
 * @param {string|object|Buffer} input Input source (file path, file descriptor, stream, or buffer)
 * @param {string|object|null} output Output destination (file path, file descriptor, stream, or null)
 * @param {object} options Normalization options
 * @returns {Promise<object>} Promise that resolves to normalization result
 */
export function normalize(input, output, options) {
  return cjs.normalize(input, output, options);
}

/**
 * Calculate PMI (Pointwise Mutual Information)
 *
 * @param {string|object|Buffer} input Input source (file path, file descriptor, stream, or buffer)
 * @param {string|object|null} output Output destination (file path, file descriptor, stream, or null)
 * @param {object} options PMI calculation options
 * @returns {Promise<object>} Promise that resolves to PMI calculation result
 */
export function pmi(input, output, options) {
  return cjs.pmi(input, output, options);
}
      `
      );
    }

    // Make CLI script executable
    makeExecutable(path.join(binDir, 'suzume-feedmill.js'));

    // Copy native binaries if they exist
    const buildDir = path.join(rootDir, 'build');
    if (fs.existsSync(buildDir)) {
      const files = fs.readdirSync(buildDir);

      // Find suzume-feedmill executable
      const exeFile = files.find(
        (file) => file === 'suzume-feedmill' || file === 'suzume-feedmill.exe'
      );

      if (exeFile) {
        const srcPath = path.join(buildDir, exeFile);
        const destPath = path.join(libDir, 'bin', exeFile);

        ensureDirExists(path.join(libDir, 'bin'));
        copyFile(srcPath, destPath);
        makeExecutable(destPath);
      }

      // Find native addon
      const addonFile = files.find((file) => file.endsWith('.node'));

      if (addonFile) {
        const srcPath = path.join(buildDir, addonFile);

        // Copy to lib/binding directory (for production)
        const libDestPath = path.join(libDir, 'binding', addonFile);
        ensureDirExists(path.join(libDir, 'binding'));
        copyFile(srcPath, libDestPath);

        // Also ensure it exists in build directory (for tests)
        const buildDestPath = path.join(buildDir, 'suzume_feedmill.node');
        if (srcPath !== buildDestPath) {
          copyFile(srcPath, buildDestPath);
        }
      } else {
        // Check in Release directory
        const releaseDir = path.join(buildDir, 'Release');
        if (fs.existsSync(releaseDir)) {
          const releaseFiles = fs.readdirSync(releaseDir);
          const releaseAddonFile = releaseFiles.find((file) => file.endsWith('.node'));

          if (releaseAddonFile) {
            const releaseSrcPath = path.join(releaseDir, releaseAddonFile);

            // Copy to lib/binding directory (for production)
            const libDestPath = path.join(libDir, 'binding', releaseAddonFile);
            ensureDirExists(path.join(libDir, 'binding'));
            copyFile(releaseSrcPath, libDestPath);

            // Also copy to build directory (for tests)
            const buildDestPath = path.join(buildDir, 'suzume_feedmill.node');
            copyFile(releaseSrcPath, buildDestPath);
          }
        }
      }
    }

    console.log('Post-build copy completed successfully');
  } catch (err) {
    console.error(`Error in post-build script: ${err.message}`);
    process.exit(1);
  }
}

// Run main function
main();
