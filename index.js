const bindings = require("./build/Release/tacvt.node");

/**
 * @typedef {Float32Array|Float64Array|Int32Array|Uint32Array|Int16Array|Uint16Array|Int8Array|Uint8Array} TypedArray
 */

/**
 * Copies values from `src` to `dst` with type conversion. Fast replacement for
 * ECMA-262 22.2.3.22.1 `%TypedArray%.prototype.set(typedarray[, offset])`.
 * @param {TypedArray} dst
 * @param {TypedArray} src
 * @param {number} [offset] Offset into the target array at which to being
 * writing values from the source array.
 * @return {void}
 */
function set(dst, src, offset = 0) {
	const dstCtor = dst.constructor;
	const srcCtor = src.constructor;
	if (srcCtor === dstCtor || srcCtor == Array) {
		return dst.set(src, offset);
	}
	const fn = `${srcCtor.name}_${dstCtor.name}`;
	switch (fn) {
		// reinterprets/memcpys (std::memcpy no faster than v8 runtime):
		case "Int32Array_Uint32Array":
		case "Uint32Array_Int32Array":
		case "Int16Array_Uint16Array":
		case "Uint16Array_Int16Array":
		case "Int8Array_Uint8Array":
		case "Uint8Array_Int8Array":
			return dst.set(src, offset);

		// conversions - implemented
		case "Float64Array_Float32Array":
		case "Float32Array_Float64Array":
		case "Float32Array_Int32Array":
		case "Int32Array_Float32Array":
			return bindings[fn](dst, src, offset);
		
		// conversions - implemented but aliased

		case "Int8Array_Int32Array": // (explicit)
		case "Int8Array_Uint32Array": // (alias)
			return bindings.Int8Array_Int32Array(dst, src, offset);

		case "Uint8Array_Int32Array": // (alias)
		case "Uint8Array_Uint32Array": // (explicit)
			return bindings.Uint8Array_Uint32Array(dst, src, offset);

		// conversions - TODO
		case "Float32Array_Uint32Array":
		case "Float32Array_Int16Array":
		case "Float32Array_Uint16Array":
		case "Float32Array_Int8Array":
		case "Float32Array_Uint8Array":

		case "Float64Array_Int32Array":
		case "Float64Array_Uint32Array":
		case "Float64Array_Int16Array":
		case "Float64Array_Uint16Array":
		case "Float64Array_Int8Array":
		case "Float64Array_Uint8Array":
		
		case "Int32Array_Float64Array":
		case "Int32Array_Int16Array":
		case "Int32Array_Int8Array":

		case "Uint32Array_Float64Array":
		case "Uint32Array_Float32Array":
		case "Uint32Array_Int16Array":
		case "Uint32Array_Uint16Array":
		case "Uint32Array_Int8Array":
		case "Uint32Array_Uint8Array":

		case "Int16Array_Float64Array":
		case "Int16Array_Float32Array":
		case "Int16Array_Int32Array":
		case "Int16Array_Uint32Array":
		case "Int16Array_Int8Array":
		case "Int16Array_Uint8Array":

		case "Uint16Array_Float64Array":
		case "Uint16Array_Float32Array":
		case "Uint16Array_Int32Array":
		case "Uint16Array_Uint32Array":
		case "Uint16Array_Int8Array":
		case "Uint16Array_Uint8Array":

		case "Int8Array_Float64Array":
		case "Int8Array_Float32Array":
		case "Int8Array_Int16Array":
		case "Int8Array_Uint16Array":

		case "Uint8Array_Float64Array":
		case "Uint8Array_Float32Array":
		case "Uint8Array_Int16Array":
		case "Uint8Array_Uint16Array":
		default:
			console.log(`Warning: fast ${fn} not implemented`);
			return dst.set(src, offset);
	}
}

module.exports = set;
