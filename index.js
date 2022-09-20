const MODULE_PATH = './build/Release/windump';

lib.default = (...args) => {
  lib.init(...args);
  return lib.deinit;
};

module.exports = lib;
