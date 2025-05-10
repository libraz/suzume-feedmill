#!/usr/bin/env node

/**
 * Incremental build script
 * Builds only the modified C++ files to save time
 */

const { execSync } = require('child_process');
const path = require('path');

// Build directory
const BUILD_DIR = path.join(__dirname, '..', 'build');

try {
  console.log('🔄 Running incremental build...');

  // Check if CMake build directory exists
  try {
    execSync(`cmake -B ${BUILD_DIR} -S .`, { stdio: 'inherit' });
  } catch (error) {
    console.error('Failed to configure CMake. Please run a full build.');
    process.exit(1);
  }

  // Build only the core library and CLI targets
  console.log('📦 Building only core library and CLI targets...');
  execSync(
    `cmake --build ${BUILD_DIR} --target suzume_feedmill_core suzume_feedmill_cli --parallel`,
    { stdio: 'inherit' }
  );

  // Build Node.js bindings
  console.log('🔗 Building Node.js bindings...');
  execSync('node-gyp rebuild', { stdio: 'inherit' });

  console.log('✅ Incremental build completed successfully!');
} catch (error) {
  console.error('❌ Error during build:', error.message);
  process.exit(1);
}
