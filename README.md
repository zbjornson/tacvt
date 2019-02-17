Quick experiment regarding fast initialization of one TypedArray from another of
a different type, e.g.

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

### How

v8 does scalar conversion with a generic conversion routine. This module uses
256-bit-wide SIMD conversions and has specialized conversion routines.

[ECMAScript's conversion routines](https://tc39.github.io/ecma262/#sec-touint32)
don't all match well with Intel's instructions and some have to be implemented
in software. (Keep that in mind if you're doing something that needs fast
conversions and don't need to adhere to the ECMAScript rules; see for example
the note in `Float32Array` to `Int32Array` below.)

### Coverage

* **Float/double conversions** are correct and fast (Intel ins'n match ECMA spec).  
  ✔️ `Float64Array` to `Float32Array`  
  ✔️ `Float32Array` to `Float64Array`

* **Float to integer conversions** require specializations; only one is done.  
  ✔️ `Float32Array` to `Int32Array` is ~~correct~~ failure around 2**32 and fairly fast.
  (Depending on the values, it's *much* faster than v8.) Intel's instructions
  don't match ECMA262 exactly: ECMA262 specifies that NaN, +Infinity and
  -Infinity to return 0, and that values wrap-around in case of overflow,
  whereas Intel's `cvt[t]ps2dq` returns 0x80000000 (-2147483648) in these cases.
  Right now this module has a fast path for when the instruction matches the
  spec (better than v8's fast path, see *TODO* below), and a slow scalar path to
  fix up values that don't.

  AVX512 `vfixupimmps` is potentially useful here but not widely available.

  I have no use case for this conversion, but if someone else does, would it be
  useful to offer fast conversion that doesn't follow ECMA262 spec and instead
  just passes through Intel's instruction behavior?

  **TODO** I think there's a missed optimization in v8's DoubleToInt32.
  Their fast-path requires this condition:
  ```cpp
  static_cast<double>(static_cast<int32_t>(double_input)) == double_input
  ```
  but I think it should be
  ```cpp
  static_cast<double>(static_cast<int32_t>(double_input)) == trunc(double_input)
  ```
  ❌ `Float32Array` to `Uint32Array` (AVX512)  
  ❌ `Float32Array` to `Int16Array`  
  ❌ `Float32Array` to `Uint16Array`  
  ❌ `Float32Array` to `Int8Array`  
  ❌ `Float32Array` to `Uint8Array`  
  ❌ `Float64Array` to `Int32Array`  
  ❌ `Float64Array` to `Uint32Array`  
  ❌ `Float64Array` to `Int16Array`  
  ❌ `Float64Array` to `Uint16Array`  
  ❌ `Float64Array` to `Int8Array`  
  ❌ `Float64Array` to `Uint8Array` require in-software specializations

* **Integer to float conversions** are correct and fast, with two exceptions.  
  ✔️ `Int32Array` to `Float64Array`  
  ✔️ `Int32Array` to `Float32Array`  
  ✔️ `Int16Array` to `Float32Array`  
  ✔️ `Uint16Array` to `Float32Array`  
  ✔️ `Int8Array` to `Float32Array`  
  ✔️ `Uint8Array` to `Float32Array`  
  ❌ `Uint32Array` to `Float32Array` and  
  ❌ `Uint32Array` to `Float64Array` require either AVX512 or in-software specializations

* **Widening integer conversions** are correct and fast.  
  ✔️ ️`Int16Array` to `Int32Array`  
  ✔️ ️`Int16Array` to `Uint32Array`  
  ✔️ ️`Uint16Array` to `Int32Array`  
  ✔️ ️`Uint16Array` to `Uint32Array`  
  ✔️ ️`Int8Array` to `Int32Array`  
  ✔️ ️`Int8Array` to `Uint32Array`  
  ✔️ ️`Int8Array` to `Int16Array`  
  ✔️ ️`Int8Array` to `Uint16Array`  
  ✔️ ️`Uint8Array` to `Int32Array`  
  ✔️ ️`Uint8Array` to `Uint32Array`  
  ✔️ ️`Uint8Array` to `Int16Array`  
  ✔️ ️`Uint8Array` to `Uint16Array`

* **Unsigned/signed conversions** are just `memcpy()`s (reinterpretations of the
  same bit strings). v8 is fast; this module passes-through to `dst.set(src)`.  
  ✔️ `Int32Array` to `Uint32Array`  
  ✔️ `Uint32Array` to `Int32Array`  
  ✔️ `Int16Array` to `Uint16Array`  
  ✔️ `Uint16Array` to `Int16Array`  
  ✔️ `Int8Array` to `Uint8Array`  
  ✔️ `Uint8Array` to `Int8Array`

* **Narrowing integer conversions** are correct and fast.  
  ✔️ `Int32Array` to `Int16Array`  
  ✔️ `Int32Array` to `Int8Array`  
  ✔️ `Uint32Array` to `Int16Array`  
  ✔️ `Uint32Array` to `Uint16Array`  
  ✔️ `Uint32Array` to `Int8Array`  
  ✔️ `Uint32Array` to `Uint8Array`  
  ✔️ `Int16Array` to `Int8Array`  
  ✔️ `Int16Array` to `Uint8Array`  
  ✔️ `Uint16Array` to `Int8Array`  
  ✔️ `Uint16Array` to `Uint8Array`

Conversions that aren't implemented pass-through to `dst.set(src)`.

### Performance

Run `node ./test.js --benchmark`.

Numbers are `dst.set(src)` (v8) ÷ `set(dst, src)` (this module).
The diagonal should be 1 or slightly less than 1; deviation from 1 there can estimate the noise in the benchmark.
I've marked ones that are actually expected to be faster with asterisks below.

```
Linux/GCC8
┌──────────────┬──────────────┬──────────────┬────────────┬─────────────┬────────────┬─────────────┬───────────┬────────────┐
│ from   \  to │ Float64Array │ Float32Array │ Int32Array │ Uint32Array │ Int16Array │ Uint16Array │ Int8Array │ Uint8Array │
├──────────────┼──────────────┼──────────────┼────────────┼─────────────┼────────────┼─────────────┼───────────┼────────────┤
│ Float64Array │     0.85     │    *4.19*    │    0.96    │    0.97     │    1.01    │    0.98     │   0.96    │    1.02    │
│ Float32Array │    *4.46*    │     1.06     │  *22.63*   │    1.02     │    0.99    │    0.99     │   1.00    │    1.01    │
│   Int32Array │    *4.43*    │    *7.18*    │    1.06    │    1.06     │  *13.71*   │  *14.65*    │ *10.57*   │   *7.19*   │
│  Uint32Array │     1.10     │     0.93     │    1.48    │    0.99     │  *11.53*   │  *10.56*    │ *12.11*   │  *11.95*   │
│   Int16Array │    *5.76*    │    *5.94*    │   *9.67*   │  *10.84*    │    0.96    │    1.00     │ *21.12*   │  *16.02*   │
│  Uint16Array │    *4.72*    │    *9.93*    │  *10.60*   │  *12.06*    │    1.02    │    1.05     │ *18.54*   │  *15.09*   │
│    Int8Array │    *2.77*    │   *12.96*    │  *11.74*   │  *10.85*    │  *25.11*   │  *21.40*    │   1.05    │    0.75    │
│   Uint8Array │    *6.38*    │   *10.49*    │  *12.32*   │   *9.86*    │  *20.77*   │  *16.01*    │   0.88    │    0.90    │
└──────────────┴──────────────┴──────────────┴────────────┴─────────────┴────────────┴─────────────┴───────────┴────────────┘
```

```
Windows/MSVS 2017
┌──────────────┬──────────────┬──────────────┬────────────┬─────────────┬────────────┬─────────────┬───────────┬────────────┐
│ from   \  to │ Float64Array │ Float32Array │ Int32Array │ Uint32Array │ Int16Array │ Uint16Array │ Int8Array │ Uint8Array │
├──────────────┼──────────────┼──────────────┼────────────┼─────────────┼────────────┼─────────────┼───────────┼────────────┤
│ Float64Array │     1.04     │    *4.64*    │    1.01    │    1.02     │    1.08    │    0.92     │   0.95    │    0.96    │
│ Float32Array │    *4.16*    │     1.09     │  *35.19*   │    1.00     │    1.04    │    0.94     │   1.01    │    1.03    │
│   Int32Array │    *4.49*    │    *6.91*    │    1.05    │    0.98     │   *8.57*   │  *11.02*    │  *9.43*   │   *9.26*   │
│  Uint32Array │     0.98     │     1.25     │    1.02    │    0.98     │   *7.32*   │   *8.28*    │  *8.45*   │   *5.30*   │
│   Int16Array │    *3.68*    │    *9.44*    │   *8.28*   │   *8.42*    │    0.95    │    0.91     │  *8.94*   │  *13.77*   │
│  Uint16Array |    *5.18*    │    *7.81*    │   *9.03*   │   *7.80*    │    1.02    │    0.80     │ *16.33*   │   *9.17*   │
│    Int8Array │    *6.21*    │    *9.95*    │   *8.10*   │   *7.27*    │  *14.54*   │   *9.84*    │   0.97    │    1.02    │
│   Uint8Array │    *3.82*    │    *9.61*    │   *9.46*   │   *9.41*    │  *14.75*   │  *14.69*    │   1.01    │    0.91    │
└──────────────┴──────────────┴──────────────┴────────────┴─────────────┴────────────┴─────────────┴───────────┴────────────┘
```

Note: Float32Array to Int32Array benchmark has almost no cases of overflow or
other fixup. Actual runtime depends on numerical values in array.

### Other TODOs

* The `offset` parameter is ignored.
* The source/destination must have a length that is a multiple of 8, 16 or 32.
  (That is, I've only dealt with the vectorized loop body and not the tail.)
* AVX2 is required, and most or all of these could be done with earlier
  extension sets albeit with narrower vectors. Since this library is just for
  fun, I have no intention of adding e.g. an SSE4.2 version.

### Why

Was a fun weekend project. I have no idea if anyone ever uses these conversions.
