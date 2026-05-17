import createMrbcModule from "./mrbc/mrbc.js";
import createMrubycModule from "./mrubyc/mrubyc.js";

(function () {
  const codeInput = document.getElementById("ruby-code");
  const output = document.getElementById("output");
  const runButton = document.getElementById("run-button");
  const status = document.getElementById("status");

  let mrbcModulePromise = null;
  let mrubycModulePromise = null;

  function setStatus(text) {
    status.textContent = text;
  }

  function appendText(text) {
    if (text == null) return;
    output.value += String(text);
    output.scrollTop = output.scrollHeight;
  }

  function appendLine(text) {
    const line = String(text || "");
    appendText(line);
    if (!line.endsWith("\n")) appendText("\n");
  }

  function clearOutput() {
    output.value = "";
  }

  function tryUnlink(fs, path) {
    try {
      fs.unlink(path);
    } catch (_) {
    }
  }

  function compileRuby(mrbcModule, rubyCode) {
    const sourceFileName = "input.rb";
    const outputFileName = "input.mrb";
    const args = ["mrbc", "-o", outputFileName, sourceFileName];
    const argPointers = [];
    let argv = 0;

    tryUnlink(mrbcModule.FS, outputFileName);
    mrbcModule.FS.writeFile(sourceFileName, rubyCode);

    try {
      argv = mrbcModule._malloc(args.length * 4);
      if (!argv) throw new Error("Failed to allocate argv.");

      for (let i = 0; i < args.length; i += 1) {
        const size = args[i].length + 1;
        const ptr = mrbcModule._malloc(size);
        if (!ptr) throw new Error("Failed to allocate an argument.");
        mrbcModule.stringToUTF8(args[i], ptr, size);
        mrbcModule.setValue(argv + i * 4, ptr, "i32");
        argPointers.push(ptr);
      }

      const result = mrbcModule._main(args.length, argv);
      if (result !== 0) {
        throw new Error("mrbc failed with exit code " + result + ".");
      }

      return mrbcModule.FS.readFile(outputFileName);
    } finally {
      for (const ptr of argPointers) {
        mrbcModule._free(ptr);
      }
      if (argv) mrbcModule._free(argv);
    }
  }

  function loadMrbcModule() {
    if (!mrbcModulePromise) {
      mrbcModulePromise = createMrbcModule({
        locateFile: function (path) {
          return "mrbc/" + path;
        },
        print: appendLine,
        printErr: appendLine,
      });
    }
    return mrbcModulePromise;
  }

  function loadMrubycModule() {
    if (!mrubycModulePromise) {
      mrubycModulePromise = createMrubycModule({
        locateFile: function (path) {
          return "mrubyc/" + path;
        },
        mrubycOutput: appendText,
        mrubycError: appendText,
        print: appendLine,
        printErr: appendLine,
      }).then(function (module) {
        module._mrbc_wasm_init();
        return module;
      });
    }
    return mrubycModulePromise;
  }

  async function runBytecode(mrubycModule, bytecode) {
    let bytecodePtr = 0;

    try {
      bytecodePtr = mrubycModule._malloc(bytecode.length);
      if (!bytecodePtr) throw new Error("Failed to allocate bytecode memory.");

      const heap = mrubycModule.HEAPU8 || new Uint8Array(mrubycModule.wasmMemory.buffer);
      heap.set(bytecode, bytecodePtr);

      const result = await mrubycModule.ccall(
        "mrbc_wasm_run",
        "number",
        ["number", "number"],
        [bytecodePtr, bytecode.length],
        { async: true },
      );

      if (result !== 0) {
        appendLine("Program exited with code " + result + ".");
      }
    } finally {
      if (bytecodePtr) mrubycModule._free(bytecodePtr);
    }
  }

  async function run() {
    runButton.disabled = true;
    clearOutput();
    setStatus("Compiling...");

    try {
      const mrbcModule = await loadMrbcModule();
      const bytecode = compileRuby(mrbcModule, codeInput.value);

      setStatus("Running...");
      const mrubycModule = await loadMrubycModule();
      await runBytecode(mrubycModule, bytecode);

      setStatus("Ready");
    } catch (error) {
      appendLine(error && error.message ? error.message : error);
      setStatus("Error");
    } finally {
      runButton.disabled = false;
    }
  }

  async function init() {
    runButton.disabled = true;

    try {
      await Promise.all([loadMrbcModule(), loadMrubycModule()]);
      setStatus("Ready");
      runButton.disabled = false;
    } catch (error) {
      appendLine(error && error.message ? error.message : error);
      setStatus("Failed to load WebAssembly modules");
    }
  }

  runButton.addEventListener("click", run);
  init();
})();
