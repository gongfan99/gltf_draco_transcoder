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

function isValidGlb(buffer) {
  // glTF 2.0 magic number: 0x46546C67 (little endian)
  const magic =
    buffer.byteLength >= 4 ? new DataView(buffer, 0, 4).getUint32(0, true) : 0;
  return magic === 0x46546c67; // "glTF"
}

const transcoder = await factory();

console.log("=== WASM Draco Transcoder Test ===");

// Use specific test files
const __dirname = dirname(fileURLToPath(import.meta.url));
const testsDir = join(__dirname, "..", "tests");
const glbFiles = ["partial_cylinder.glb"];

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
    const compressTime = (compressEnd - compressStart).toFixed(2);

    const compressedSize = compressedArrayBuffer.byteLength;

    // Decompress to verify round-trip
    const decompressStart = performance.now();
    const decompressedArrayBuffer = transcoder.decompress_gltf(
      compressedArrayBuffer
    );
    const decompressEnd = performance.now();
    const decompressTime = (decompressEnd - decompressStart).toFixed(2);

    const decompressedSize = decompressedArrayBuffer.byteLength;

    // Calculate statistics
    const compressionRatio = (
      (1 - compressedSize / originalSize) *
      100
    ).toFixed(1);

    console.log(`  Compress time: ${compressTime} ms`);
    console.log(`  Decompress time: ${decompressTime} ms`);

    // Verify round-trip integrity
    const originalData = new Uint8Array(buffer);
    const decompressedData = new Uint8Array(decompressedArrayBuffer);

    // Check if decompressed data is a valid glb file
    const isValid = isValidGlb(decompressedArrayBuffer);
    console.log(`  Valid glb file: ${isValid ? "✅ Yes" : "❌ No"}`);

    // Basic sanity check - sizes should be similar (allowing for minor format differences)
    const sizeDifference = Math.abs(decompressedSize - originalSize);
    const maxAcceptableDifference = Math.min(originalSize * 0.5, 10240);

    if (!isValid) {
      console.error(
        `❌ ${filename}: Round-trip failed - decompressed data is not a valid glb file`
      );
    } else if (sizeDifference > maxAcceptableDifference) {
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
