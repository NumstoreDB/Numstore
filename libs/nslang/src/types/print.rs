// Prints the bytes of one element of a type as text

use std::fmt::{self, Write};

use crate::types::types::{Prim, SArray, Type};

/// Prints one element of [t] from [buf] then a newline
/// Arrays show at most [max_elems] per dimension
/// Bytes missing from [buf] print as ?
pub fn print_data(w: &mut dyn Write, buf: &[u8], t: &Type<'_>, max_elems: u32) -> fmt::Result {
    print_type(w, buf, t, max_elems, 0)?;
    w.write_char('\n')
}

/// c's %g
pub fn fmt_g(v: f64) -> String {
    if v.is_nan() {
        return "nan".into();
    }
    if v.is_infinite() {
        return if v < 0.0 { "-inf" } else { "inf" }.into();
    }
    if v == 0.0 {
        return if v.is_sign_negative() { "-0" } else { "0" }.into();
    }

    fn trim(s: &str) -> &str {
        if s.contains('.') {
            s.trim_end_matches('0').trim_end_matches('.')
        } else {
            s
        }
    }

    // Round to six significant digits first: that decides the exponent
    let sci = format!("{v:.5e}");
    let (mantissa, exp) = sci.split_once('e').expect("{:e} always has an exponent");
    let exp: i32 = exp.parse().expect("{:e} exponent is an integer");

    if !(-4..6).contains(&exp) {
        let sign = if exp < 0 { '-' } else { '+' };
        format!("{}e{sign}{:02}", trim(mantissa), exp.abs())
    } else {
        let precision = (5 - exp) as usize;
        trim(&format!("{v:.precision$}")).to_owned()
    }
}

/// IEEE 754 half to single precision
pub fn f16_to_f32(h: u16) -> f32 {
    let sign = u32::from(h >> 15) << 31;
    let exp = u32::from(h >> 10) & 0x1f;
    let frac = u32::from(h) & 0x3ff;

    let bits = match (exp, frac) {
        (0, 0) => sign,
        // Subnormal - shift the fraction until the leading bit is implicit
        (0, _) => {
            let shift = frac.leading_zeros() - 21;
            let frac = (frac << shift) & 0x3ff;
            sign | ((113 - shift) << 23) | (frac << 13)
        }
        (0x1f, _) => sign | 0x7f80_0000 | (frac << 13),
        _ => sign | ((exp + 112) << 23) | (frac << 13),
    };

    f32::from_bits(bits)
}

fn indent(w: &mut dyn Write, spaces: u32) -> fmt::Result {
    (0..spaces).try_for_each(|_| w.write_char(' '))
}

fn at(buf: &[u8], ofst: usize) -> &[u8] {
    buf.get(ofst..).unwrap_or(&[])
}

fn print_prim(w: &mut dyn Write, buf: &[u8], p: Prim) -> fmt::Result {
    // A native endian number at [ofst]
    macro_rules! num {
        ($t:ty, $ofst:expr) => {{
            const N: usize = std::mem::size_of::<$t>();
            match buf.get($ofst..$ofst + N) {
                Some(b) => <$t>::from_ne_bytes(b.try_into().expect("slice is N bytes")),
                None => return w.write_char('?'),
            }
        }};
    }
    // A complex number - two numbers back to back
    macro_rules! pair {
        ($t:ty) => {{
            let (r, im) = (num!($t, 0), num!($t, std::mem::size_of::<$t>()));
            write!(w, "({r}, {im})")
        }};
    }
    // No portable 128 bit float - print the bits
    macro_rules! f128_hex {
        ($ofst:expr) => {{
            let (lo, hi) = (num!(u64, $ofst), num!(u64, $ofst + 8));
            format!("<f128:0x{hi:016x}{lo:016x}>")
        }};
    }

    match p {
        Prim::U8 => write!(w, "{}", num!(u8, 0)),
        Prim::U16 => write!(w, "{}", num!(u16, 0)),
        Prim::U32 => write!(w, "{}", num!(u32, 0)),
        Prim::U64 => write!(w, "{}", num!(u64, 0)),
        Prim::I8 => write!(w, "{}", num!(i8, 0)),
        Prim::I16 => write!(w, "{}", num!(i16, 0)),
        Prim::I32 => write!(w, "{}", num!(i32, 0)),
        Prim::I64 => write!(w, "{}", num!(i64, 0)),

        Prim::F16 => w.write_str(&fmt_g(f16_to_f32(num!(u16, 0)).into())),
        Prim::F32 => w.write_str(&fmt_g(num!(f32, 0).into())),
        Prim::F64 => w.write_str(&fmt_g(num!(f64, 0))),
        Prim::F128 => w.write_str(&f128_hex!(0)),

        Prim::CF32 => {
            let (r, im) = (num!(u16, 0), num!(u16, 2));
            let (r, im) = (f16_to_f32(r), f16_to_f32(im));
            write!(w, "({}, {})", fmt_g(r.into()), fmt_g(im.into()))
        }
        Prim::CF64 => {
            let (r, im) = (num!(f32, 0), num!(f32, 4));
            write!(w, "({}, {})", fmt_g(r.into()), fmt_g(im.into()))
        }
        Prim::CF128 => {
            let (r, im) = (num!(f64, 0), num!(f64, 8));
            write!(w, "({}, {})", fmt_g(r), fmt_g(im))
        }
        Prim::CF256 => write!(w, "({}, {})", f128_hex!(0), f128_hex!(16)),

        Prim::CI16 => pair!(i8),
        Prim::CI32 => pair!(i16),
        Prim::CI64 => pair!(i32),
        Prim::CI128 => pair!(i64),
        Prim::CU16 => pair!(u8),
        Prim::CU32 => pair!(u16),
        Prim::CU64 => pair!(u32),
        Prim::CU128 => pair!(u64),
    }
}

/// Product of dims[dim+1 .. rank-1] * element size
fn sarray_sub_size(sa: &SArray<'_>, dim: usize) -> usize {
    sa.dims[dim + 1..]
        .iter()
        .fold(sa.t.byte_size(), |acc, &d| acc.wrapping_mul(d)) as usize
}

/// [col] is the column of the '[' just printed - continuation rows align under it
fn print_sarray_dim(
    w: &mut dyn Write,
    buf: &[u8],
    sa: &SArray<'_>,
    dim: usize,
    max_elems: u32,
    ind: u32,
    col: u32,
) -> fmt::Result {
    let len = sa.dims[dim];
    let show = len.min(max_elems) as usize;
    let sub_size = sarray_sub_size(sa, dim);

    w.write_char('[')?;

    if dim + 1 == sa.dims.len() {
        // Innermost dimension: elements inline
        for i in 0..show {
            if i > 0 {
                w.write_str(", ")?;
            }
            print_type(w, at(buf, i * sub_size), &sa.t, max_elems, ind + 1)?;
        }
        if len > max_elems {
            w.write_str(", ...")?;
        }
    } else {
        // Outer dimension: each sub-array on its own line
        for i in 0..show {
            if i > 0 {
                w.write_str(",\n")?;
                indent(w, col + 1)?;
            }
            print_sarray_dim(
                w,
                at(buf, i * sub_size),
                sa,
                dim + 1,
                max_elems,
                ind + 1,
                col + 1,
            )?;
        }
        if len > max_elems {
            w.write_str(",\n")?;
            indent(w, col + 1)?;
            w.write_str("...")?;
        }
    }

    w.write_char(']')
}

fn print_type(
    w: &mut dyn Write,
    buf: &[u8],
    t: &Type<'_>,
    max_elems: u32,
    ind: u32,
) -> fmt::Result {
    match t {
        Type::Prim(p) => print_prim(w, buf, *p),

        Type::Struct(s) => {
            w.write_str("{\n")?;
            let mut ofst = 0usize;
            let field_ind = ind + 4;
            for (i, (key, ft)) in s.keys.iter().zip(s.types.iter()).enumerate() {
                indent(w, field_ind)?;
                write!(w, "{key} = ")?;
                match ft {
                    Type::SArray(sa) if !sa.dims.is_empty() => {
                        let col = field_ind + key.len() as u32 + 3;
                        print_sarray_dim(w, at(buf, ofst), sa, 0, max_elems, field_ind, col)?;
                    }
                    _ => print_type(w, at(buf, ofst), ft, max_elems, field_ind)?,
                }
                ofst = ofst.wrapping_add(ft.byte_size() as usize);
                if i + 1 < s.keys.len() {
                    w.write_char(',')?;
                }
                w.write_char('\n')?;
            }
            indent(w, ind)?;
            w.write_char('}')
        }

        // Print the bytes as the first member
        Type::Union(u) => {
            w.write_str("<union[0]: ")?;
            match (u.keys.first(), u.types.first()) {
                (Some(key), Some(ft)) => {
                    write!(w, "{key} = ")?;
                    print_type(w, buf, ft, max_elems, ind)?;
                }
                _ => w.write_str("empty")?,
            }
            w.write_char('>')
        }

        Type::SArray(sa) if sa.dims.is_empty() => w.write_str("[]"),
        Type::SArray(sa) => print_sarray_dim(w, buf, sa, 0, max_elems, ind, ind),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::compiler::compile_type;

    fn printed(src: &str, buf: &[u8], max_elems: u32) -> String {
        let t = compile_type(src).unwrap();
        assert!(buf.len() >= t.byte_size() as usize, "test buffer too small");
        let mut out = String::new();
        print_data(&mut out, buf, &t, max_elems).unwrap();
        out
    }

    fn bytes<T: Copy, const N: usize>(vals: [T; N], f: impl Fn(T) -> Vec<u8>) -> Vec<u8> {
        vals.iter().flat_map(|&v| f(v)).collect()
    }

    #[test]
    fn percent_g() {
        for (v, exp) in [
            (0.0, "0"),
            (-0.0, "-0"),
            (1.0, "1"),
            (-1.5, "-1.5"),
            (2.5, "2.5"),
            (0.1, "0.1"),
            (100000.0, "100000"),
            (123456.0, "123456"),
            (1234567.0, "1.23457e+06"),
            (999999.5, "1e+06"),
            (1e6, "1e+06"),
            (0.0001, "0.0001"),
            (0.00001234, "1.234e-05"),
            (1.23456789, "1.23457"),
            (1e100, "1e+100"),
            (1.5e-10, "1.5e-10"),
            (f64::INFINITY, "inf"),
            (f64::NEG_INFINITY, "-inf"),
            (f64::NAN, "nan"),
        ] {
            assert_eq!(fmt_g(v), exp, "{v:?}");
        }
    }

    #[test]
    fn half_floats() {
        for (h, exp) in [
            (0x0000u16, 0.0f32),
            (0x3c00, 1.0),
            (0xc000, -2.0),
            (0x3e00, 1.5),
            (0x7bff, 65504.0),
            (0x0400, 6.103_515_6e-5), // smallest normal
            (0x0001, 5.960_464_5e-8), // smallest subnormal
            (0x03ff, 6.097_555e-5),   // largest subnormal
        ] {
            assert_eq!(f16_to_f32(h), exp, "{h:#06x}");
        }
        assert_eq!(f16_to_f32(0x7c00), f32::INFINITY);
        assert_eq!(f16_to_f32(0xfc00), f32::NEG_INFINITY);
        assert!(f16_to_f32(0x7e00).is_nan());
        assert!(f16_to_f32(0x8000).is_sign_negative());
    }

    /// Port of TEST (print_prim_value): every primitive
    #[test]
    fn prims() {
        assert_eq!(printed("u8", &[200], 10), "200\n");
        assert_eq!(printed("u16", &60000u16.to_ne_bytes(), 10), "60000\n");
        assert_eq!(
            printed("u32", &4000000000u32.to_ne_bytes(), 10),
            "4000000000\n"
        );
        assert_eq!(
            printed("u64", &u64::MAX.to_ne_bytes(), 10),
            "18446744073709551615\n"
        );
        assert_eq!(printed("i8", &(-5i8).to_ne_bytes(), 10), "-5\n");
        assert_eq!(printed("i16", &(-300i16).to_ne_bytes(), 10), "-300\n");
        assert_eq!(printed("i32", &(-42i32).to_ne_bytes(), 10), "-42\n");
        assert_eq!(
            printed("i64", &i64::MIN.to_ne_bytes(), 10),
            "-9223372036854775808\n"
        );

        assert_eq!(printed("f16", &0x3e00u16.to_ne_bytes(), 10), "1.5\n");
        assert_eq!(printed("f32", &1.5f32.to_ne_bytes(), 10), "1.5\n");
        assert_eq!(printed("f64", &(-2.25f64).to_ne_bytes(), 10), "-2.25\n");

        let f128 = bytes([0x1122334455667788u64, 0x99aabbccddeeff00], |v| {
            v.to_ne_bytes().to_vec()
        });
        assert_eq!(
            printed("f128", &f128, 10),
            "<f128:0x99aabbccddeeff001122334455667788>\n"
        );

        let cf32 = bytes([0x3c00u16, 0xc000], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cf32", &cf32, 10), "(1, -2)\n");
        let cf64 = bytes([1.5f32, -0.5], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cf64", &cf64, 10), "(1.5, -0.5)\n");
        let cf128 = bytes([1e10f64, 2.0], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cf128", &cf128, 10), "(1e+10, 2)\n");
        assert_eq!(
            printed("cf256", &[0u8; 32], 10),
            "(<f128:0x00000000000000000000000000000000>, \
             <f128:0x00000000000000000000000000000000>)\n"
        );

        assert_eq!(printed("ci16", &[0xff, 2], 10), "(-1, 2)\n");
        let ci32 = bytes([-3i16, 4], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("ci32", &ci32, 10), "(-3, 4)\n");
        let ci64 = bytes([-5i32, 6], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("ci64", &ci64, 10), "(-5, 6)\n");
        let ci128 = bytes([-7i64, 8], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("ci128", &ci128, 10), "(-7, 8)\n");

        assert_eq!(printed("cu16", &[255, 2], 10), "(255, 2)\n");
        let cu32 = bytes([65535u16, 4], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cu32", &cu32, 10), "(65535, 4)\n");
        let cu64 = bytes([5u32, 6], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cu64", &cu64, 10), "(5, 6)\n");
        let cu128 = bytes([7u64, 8], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("cu128", &cu128, 10), "(7, 8)\n");
    }

    /// Port of TEST (type_print_data) and (print_type_inner)
    #[test]
    fn structs_and_unions() {
        let buf = bytes([1.5f32, 2.5], |v| v.to_ne_bytes().to_vec());
        assert_eq!(
            printed("struct { x f32, y f32 }", &buf, 10),
            "{\n    x = 1.5,\n    y = 2.5\n}\n"
        );

        // A struct with an array field
        let mut buf = 7u32.to_ne_bytes().to_vec();
        buf.extend(bytes([1i32, 2, 3], |v| v.to_ne_bytes().to_vec()));
        assert_eq!(
            printed("struct { tag u32, values [3]i32 }", &buf, 10),
            "{\n    tag = 7,\n    values = [1, 2, 3]\n}\n"
        );

        // A union shows its bytes as the first member
        assert_eq!(
            printed(
                "union { as_int i64, as_uint u64 }",
                &(-1i64).to_ne_bytes(),
                10
            ),
            "<union[0]: as_int = -1>\n"
        );

        // Nested structs indent
        let buf = [1u8, 2];
        assert_eq!(
            printed("struct { a u8, b struct { c u8 } }", &buf, 10),
            "{\n    a = 1,\n    b = {\n        c = 2\n    }\n}\n"
        );
    }

    #[test]
    fn empty_union_and_rankless_array_do_not_panic() {
        use crate::types::types::{sarray, union};
        let mut out = String::new();
        print_data(&mut out, &[0], &union(vec![]), 10).unwrap();
        assert_eq!(out, "<union[0]: empty>\n");

        let mut out = String::new();
        print_data(&mut out, &[0], &sarray(&[], Type::Prim(Prim::U8)), 10).unwrap();
        assert_eq!(out, "[]\n");
    }

    /// Port of TEST (print_sarray_dim) and (sarray_sub_size)
    #[test]
    fn arrays() {
        let buf = bytes([1i32, 2, 3], |v| v.to_ne_bytes().to_vec());
        assert_eq!(printed("[3]i32", &buf, 10), "[1, 2, 3]\n");

        // Truncated to max_elems
        let buf: Vec<u8> = (0..10i32).flat_map(|v| v.to_ne_bytes()).collect();
        assert_eq!(printed("[10]i32", &buf, 3), "[0, 1, 2, ...]\n");

        // Two dimensions: one row per line, lined up under the first
        let buf: Vec<u8> = (0..6).collect();
        assert_eq!(printed("[2][3]u8", &buf, 10), "[[0, 1, 2],\n [3, 4, 5]]\n");

        // Two dimensions, both truncated
        let buf: Vec<u8> = (0..20).collect();
        assert_eq!(
            printed("[5][4]u8", &buf, 2),
            "[[0, 1, ...],\n [4, 5, ...],\n ...]\n"
        );

        // Three dimensions
        let buf: Vec<u8> = (0..8).collect();
        assert_eq!(
            printed("[2][2][2]u8", &buf, 10),
            "[[[0, 1],\n  [2, 3]],\n [[4, 5],\n  [6, 7]]]\n"
        );

        let t = compile_type("[2][3][4]u16").unwrap();
        let Type::SArray(sa) = &t else { unreachable!() };
        assert_eq!(sarray_sub_size(sa, 0), 3 * 4 * 2);
        assert_eq!(sarray_sub_size(sa, 1), 4 * 2);
        assert_eq!(sarray_sub_size(sa, 2), 2);
    }

    /// Port of TEST (print_indent)
    #[test]
    fn indents() {
        let mut out = String::new();
        indent(&mut out, 0).unwrap();
        assert_eq!(out, "");
        indent(&mut out, 3).unwrap();
        assert_eq!(out, "   ");
    }

    // A short buffer prints ? for the missing bytes
    #[test]
    fn short_buffer() {
        let t = compile_type("struct { a u8, b u32 }").unwrap();
        let mut out = String::new();
        print_data(&mut out, &[9], &t, 10).unwrap();
        assert_eq!(out, "{\n    a = 9,\n    b = ?\n}\n");
    }
}
