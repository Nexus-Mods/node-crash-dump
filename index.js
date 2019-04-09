const MODULE_PATH = './build/Release/windump';

const lib = process.platform === 'win32' ? require(MODULE_PATH) : {
  init: () => undefined,
};

lib.default = () => {
  lib.init();
  return lib.deinit;
};

module.exports = lib;
