const assert = require("assert");
const set = require(".");

const types = [
	Float64Array,
	Float32Array,
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

const doBench = process.argv.includes("--benchmark");

const size = doBench ? 32 * 10 * 1024 : 64;
const times = doBench ? 1000 : 1;
const profiles = {};

for (const From of types) {
	profiles[From.name] = {};
	for (const To of types) {
		const src = new From(size);
		const dst1 = new To(size);
		const dst2 = new To(size);
		for (let i = 0; i < size; i++) src[i] = Math.random() * 262144 * pm();
		// Some special values:
		src[0] = -127;
		src[1] = 0;
		src[2] = -1;
		src[3] = 1;
		src[4] = 255;
		src[5] = Infinity;
		src[6] = -Infinity;
		src[7] = NaN;
		src[8] = 1.1;
		src[9] = 1.9;
		src[10] = -1.1;
		src[11] = -1.9;
	
		// src[12] = 2**32;
		// src[13] = 2**32 - 1;
		// src[14] = 2**32 + 1;
	
		src[15] = 2**16 - 1;
		src[16] = 2**16;
		src[17] = 2**16 + 1;
		src[18] = (-2)**16 - 1;
		src[19] = (-2)**16;
		src[20] = (-2)**16 + 1;

		let start = process.hrtime.bigint();
		for (let i = 0; i < times; i++) dst1.set(src);
		const v8Time = Number(process.hrtime.bigint() - start);

		start = process.hrtime.bigint();
		for (let i = 0; i < times; i++) set(dst2, src);
		const tacvtTime = Number(process.hrtime.bigint() - start);

		profiles[From.name][To.name] = Math.round(v8Time / tacvtTime * 100) / 100;

		if (!doBench) {
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
}

if (doBench) {
	console.log("Row = from, Col = to");
	console.log("The diagonal should be 1; deviation from 1 there can estimate the noise in the benchmark.")
	console.table(profiles);
}
