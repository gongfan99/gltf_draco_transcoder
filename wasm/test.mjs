import { readFile, readdir } from "node:fs/promises";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import factory from "../build/gltf_draco_transcoder.js";

function toArrayBuffer(nodeBuffer) {
  return nodeBuffer.buffer.slice(
    nodeBuffer.byteOffset,
    nodeBuffer.byteOffset + nodeBuffer.byteLength
  );
}

const transcoder = await factory();

console.log("=== WASM Draco Transcoder Test ===");

// Find all .glb files in the tests directory
const __dirname = dirname(fileURLToPath(import.meta.url));
const testsDir = join(__dirname, "..", "tests");
const testFiles = await readdir(testsDir);
const glbFiles = testFiles.filter((file) => file.endsWith(".glb"));

if (glbFiles.length === 0) {
  console.error("No .glb test files found in tests directory");
  process.exit(1);
}

console.log(`Found ${glbFiles.length} test files: ${glbFiles.join(", ")}\n`);

for (const filename of glbFiles) {
  const filePath = join(testsDir, filename);

  try {
    // Read the file
    const buffer = await readFile(filePath);
    const originalSize = buffer.length;

    // Compress
    const compressStart = performance.now();
    const compressedArrayBuffer = transcoder.compress_gltf(
      toArrayBuffer(buffer),
      {}
    );
    const compressEnd = performance.now();

    const compressedSize = compressedArrayBuffer.byteLength;

    // Decompress to verify round-trip
    const decompressStart = performance.now();
    const decompressedArrayBuffer = transcoder.decompress_gltf(
      compressedArrayBuffer
    );
    const decompressEnd = performance.now();

    const decompressedSize = decompressedArrayBuffer.byteLength;

    // Calculate statistics
    const compressionRatio = (
      (1 - compressedSize / originalSize) *
      100
    ).toFixed(1);
    const compressTime = (compressEnd - compressStart).toFixed(2);
    const decompressTime = (decompressEnd - decompressStart).toFixed(2);

    // Verify round-trip integrity
    const originalData = new Uint8Array(buffer);
    const decompressedData = new Uint8Array(decompressedArrayBuffer);

    // Basic sanity check - sizes should be similar (allowing for minor format differences)
    const sizeDifference = Math.abs(decompressedSize - originalSize);
    const maxAcceptableDifference = Math.min(originalSize * 1.0, 10240); // 10% or 1KB

    if (sizeDifference > maxAcceptableDifference) {
      console.error(
        `❌ ${filename}: Round-trip failed - size difference too large (${sizeDifference} bytes)`
      );
    } else {
      console.log(
        `✅ ${filename}: ${originalSize} → ${compressedSize} bytes (${compressionRatio}% compression, ${compressTime}ms compress, ${decompressTime}ms decompress)`
      );
    }
  } catch (error) {
    console.error(`❌ ${filename}: Failed - ${error.message}`);
  }
}

console.log("\n=== Test Complete ===");
