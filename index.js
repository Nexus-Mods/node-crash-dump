const MODULE_PATH = './build/Release/windump';

const lib = process.platform === 'win32' ? require(MODULE_PATH) : {
  init: () => undefined,
  deinit: () => {},
};

/**
 * Initialize crash dump handling.
 * @param {string} dmpPath - Path where crash dumps will be written
 * @returns {() => void} Function to call to deinitialize crash dump handling
 */
function init(dmpPath) {
  lib.init(dmpPath);
  return lib.deinit;
}

module.exports = init;
module.exports.default = init;
