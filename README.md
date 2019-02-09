Quick experiment regarding fast initialization of one TypedArray from another of a different type, e.g.

```js
const inp = new Float32Array(len);
const out = new Uint16Array(inp);
```

or

```js
const inp = new Int8Array(len);
const out = new Float32Array(len);
out.set(inp);
```

v8 currently does scalar conversion. This module uses 256-bit-wide conversions and can get close to a linear speedup with the number of vector elements.

Far from complete. I haven't looked too closely at differences between [ECMAScript's conversion routines](https://tc39.github.io/ecma262/#sec-touint32) and Intel's instructions.
