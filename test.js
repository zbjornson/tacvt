const assert = require("assert");
const set = require(".");

const types = [
	Float32Array,
	Float64Array,
	Int32Array,
	Uint32Array,
	Int16Array,
	Uint16Array,
	Int8Array,
	Uint8Array
];

function pm() {
	return Math.random < 0.5 ? -1 : 1;
}

for (const From of types) {
	for (const To of types) {
		const src = new From(8 * 8);
		const dst1 = new To(8 * 8);
		const dst2 = new To(8 * 8);
		for (let i = 0; i < src.length; i++) src[i] = Math.random() * 262144 * pm();
		src[0] = -127;
		dst1.set(src);
		set(dst2, src);
		try {
			assert.deepStrictEqual(dst1, dst2, `${From.name} to ${To.name} failed`);
			console.log(`${From.name} to ${To.name} okay`);
		} catch (ex) {
			for (let i = 0; i < src.length; i++) {
				if (dst1[i] !== dst2[i]) console.log(src[i], dst1[i], dst2[i]);
			}
			throw ex;
		}
	}
}
