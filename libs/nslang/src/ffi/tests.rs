// Tests of the c functions - called like c calls them

use std::alloc::Layout;
use std::cell::{Cell, RefCell};
use std::ffi::CString;

use super::*;

// =============================================================================
// Fixtures
// =============================================================================

// The arena behind nslang_alloc - frees everything on drop
// fail_after(n) fails every allocation after the next [n]
struct TestArena {
    blocks: RefCell<Vec<(*mut u8, Layout)>>,
    budget: Cell<Option<usize>>,
    // Return pointers one byte off
    misalign: Cell<bool>,
}

impl TestArena {
    fn new() -> Box<TestArena> {
        Box::new(TestArena {
            blocks: RefCell::new(Vec::new()),
            budget: Cell::new(None),
            misalign: Cell::new(false),
        })
    }

    fn fail_after(&self, n: usize) {
        self.budget.set(Some(n));
    }

    fn unlimited(&self) {
        self.budget.set(None);
    }

    fn allocations(&self) -> usize {
        self.blocks.borrow().len()
    }

    fn alloc(&self) -> NslangAlloc {
        NslangAlloc {
            ctx: self as *const TestArena as *mut c_void,
            alloc: Some(Self::callback),
        }
    }

    unsafe extern "C" fn callback(ctx: *mut c_void, size: usize, align: usize) -> *mut c_void {
        let arena = unsafe { &*(ctx as *const TestArena) };

        if let Some(left) = arena.budget.get() {
            if left == 0 {
                return ptr::null_mut();
            }
            arena.budget.set(Some(left - 1));
        }

        let layout = Layout::from_size_align(size.max(1) + 16, align.max(8)).unwrap();
        let p = unsafe { std::alloc::alloc(layout) };
        arena.blocks.borrow_mut().push((p, layout));

        if arena.misalign.get() {
            unsafe { p.add(1).cast() }
        } else {
            p.cast()
        }
    }
}

impl Drop for TestArena {
    fn drop(&mut self) {
        for (p, layout) in self.blocks.borrow_mut().drain(..) {
            unsafe { std::alloc::dealloc(p, layout) };
        }
    }
}

fn cstr(s: &str) -> CString {
    CString::new(s).unwrap()
}

fn s(text: &'static str) -> CStr32 {
    CStr32 {
        len: text.len() as u32,
        data: text.as_ptr().cast(),
    }
}

fn str_of(s: &CStr32) -> &str {
    unsafe { lift_str(s) }.unwrap()
}

fn zeroed<T>() -> T {
    unsafe { mem::zeroed() }
}

fn prim(p: Prim) -> CType {
    let mut t: CType = zeroed();
    t.tag = T_PRIM;
    t.u.p = p.code().into();
    t
}

fn compile_type(a: &TestArena, src: &str) -> CType {
    let mut dest: CType = zeroed();
    let mut e = NslangError::new();
    let src = cstr(src);
    let ret = unsafe { nslang_compile_type(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_OK, "{}", e.message());
    dest
}

fn compile_query(a: &TestArena, src: &str) -> CQuery {
    let mut dest: CQuery = zeroed();
    let mut e = NslangError::new();
    let src = cstr(src);
    let ret = unsafe { nslang_compile_query(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_OK, "{}", e.message());
    dest
}

fn compile_subtype(a: &TestArena, src: &str) -> CSubtype {
    let mut dest: CSubtype = zeroed();
    let mut e = NslangError::new();
    let src = cstr(src);
    let ret = unsafe { nslang_compile_subtype(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_OK, "{}", e.message());
    dest
}

fn compile_type_ref(a: &TestArena, src: &str) -> CTypeRef {
    let mut dest: CTypeRef = zeroed();
    let mut e = NslangError::new();
    let src = cstr(src);
    let ret = unsafe { nslang_compile_type_ref(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_OK, "{}", e.message());
    dest
}

fn compile_user_stride(src: &str) -> std::result::Result<CUserStride, i32> {
    let mut dest: CUserStride = zeroed();
    let mut e = NslangError::new();
    let src = cstr(src);
    match unsafe { nslang_compile_user_stride(&mut dest, src.as_ptr(), &mut e) } {
        NSLANG_OK => Ok(dest),
        code => {
            assert_eq!(code, e.code);
            assert!(e.len > 0);
            Err(code)
        }
    }
}

fn snprintf(t: &CType) -> String {
    let len = unsafe { type_snprintf(ptr::null_mut(), 0, t) };
    assert!(len >= 0);
    let mut buf = vec![0u8; len as usize + 1];
    let again = unsafe { type_snprintf(buf.as_mut_ptr().cast(), buf.len() as u32, t) };
    assert_eq!(again, len);
    assert_eq!(buf.pop(), Some(0));
    String::from_utf8(buf).unwrap()
}

// [start:stop:step] constructors, ports of the ones in ns_stride.h

fn us(start: i64, stop: i64, step: i64, present: u32) -> CUserStride {
    CUserStride {
        start,
        step,
        stop,
        present,
    }
}

fn ustride() -> CUserStride {
    us(0, 0, 0, COLON_PRESENT)
}

fn ustride_single(start: i64) -> CUserStride {
    us(start, 0, 0, START_PRESENT)
}

fn ustride012(start: i64, stop: i64, step: i64) -> CUserStride {
    us(
        start,
        stop,
        step,
        START_PRESENT | STOP_PRESENT | STEP_PRESENT | COLON_PRESENT,
    )
}

// =============================================================================
// Layout
// =============================================================================

// The sizes a 64 bit c compiler gives the numstore structs
#[cfg(target_pointer_width = "64")]
#[test]
fn layouts_match_the_header() {
    assert_eq!(mem::size_of::<CStr32>(), 16);
    assert_eq!(mem::size_of::<CUserStride>(), 32);
    assert_eq!(mem::size_of::<CMultiUserStride>(), 16);
    assert_eq!(mem::size_of::<CFields>(), 24);
    assert_eq!(mem::size_of::<CSArray>(), 24);
    assert_eq!(mem::size_of::<CType>(), 32);
    assert_eq!(mem::size_of::<CTypeAccessor>(), 32);
    assert_eq!(mem::size_of::<CSubtype>(), 48);
    assert_eq!(mem::size_of::<CTypeRef>(), 56);
    assert_eq!(mem::size_of::<CQuery>(), 56);
    assert_eq!(mem::size_of::<NslangError>(), 264);

    assert_eq!(mem::offset_of!(CType, u), 8);
    assert_eq!(mem::offset_of!(CFields, keys), 8);
    assert_eq!(mem::offset_of!(CSArray, dims), 8);
    assert_eq!(mem::offset_of!(CUserStride, present), 24);
    assert_eq!(mem::offset_of!(CQuery, u), 8);
    assert_eq!(mem::offset_of!(CCreateQuery, ty), 16);
    assert_eq!(mem::offset_of!(CNamedQuery, if_exists), 16);
    assert_eq!(mem::offset_of!(CHelpQuery, command), 4);
}

// =============================================================================
// Words
// =============================================================================

/// Port of TEST (lex_is_keyword)
#[test]
fn keywords() {
    let is = |w: &str, len: u32| unsafe { lex_is_keyword(w.as_ptr().cast(), len) };

    for w in [
        "create", "delete", "if", "exists", "struct", "union", "true", "false",
    ] {
        assert!(is(w, w.len() as u32), "{w}");
    }
    for w in ["u8", "i8", "f64", "cf128"] {
        assert!(is(w, w.len() as u32), "{w}");
    }
    for w in ["foo", "_x", "iff", "u8x", "creates", ""] {
        assert!(!is(w, w.len() as u32), "{w}");
    }

    // The length is what is read, not the terminator
    assert!(is("ifx", 2));

    assert!(!unsafe { lex_is_keyword(ptr::null(), 0) });
}

#[test]
fn string_to_prim() {
    for &p in Prim::ALL {
        let name = p.as_str();
        let code = unsafe { nslang_strtoprim(name.as_ptr().cast(), name.len() as u32) };
        assert_eq!(code, i32::from(p.code()));
    }

    for w in ["", "u9", "U8", "u8 ", "struct"] {
        assert_eq!(
            unsafe { nslang_strtoprim(w.as_ptr().cast(), w.len() as u32) },
            -1,
            "{w:?}"
        );
    }

    // Only [len] bytes are read
    assert_eq!(
        unsafe { nslang_strtoprim("u16x".as_ptr().cast(), 3) },
        i32::from(Prim::U16.code())
    );
}

// =============================================================================
// Strides
// =============================================================================

/// Port of TEST (compile_user_stride)
#[test]
fn user_strides() {
    for src in ["[]", "[ ]", " [ ]", "[", "[1", "[a]", "[1:2:0]", "1]"] {
        assert_eq!(compile_user_stride(src), Err(NSLANG_ERR_SYNTAX), "{src}");
    }

    let c = COLON_PRESENT;
    let (a, o, e) = (START_PRESENT, STOP_PRESENT, STEP_PRESENT);
    let cases = [
        ("[5]", us(5, 0, 0, a)),
        ("[-5]", us(-5, 0, 0, a)),
        ("[:]", us(0, 0, 0, c)),
        ("[:6]", us(0, 6, 0, c | o)),
        ("[5:]", us(5, 0, 0, c | a)),
        ("[5:6]", us(5, 6, 0, c | a | o)),
        ("[::]", us(0, 0, 0, c)),
        ("[::7]", us(0, 0, 7, c | e)),
        ("[:6:]", us(0, 6, 0, c | o)),
        ("[:6:7]", us(0, 6, 7, c | o | e)),
        ("[5::]", us(5, 0, 0, c | a)),
        ("[5::7]", us(5, 0, 7, c | a | e)),
        ("[5:6:]", us(5, 6, 0, c | a | o)),
        ("[5:6:7]", us(5, 6, 7, c | a | o | e)),
        ("[-5:-6:-7]", us(-5, -6, -7, c | a | o | e)),
    ];
    for (src, expected) in cases {
        let actual = compile_user_stride(src).unwrap();
        assert!(
            unsafe { user_stride_equal(&actual, &expected) },
            "{src}: {actual:?}"
        );

        // What goes out comes back in
        assert_eq!(lower_stride(&lift_stride(&actual).unwrap()), actual);
    }
}

#[test]
fn user_stride_equality() {
    let base = ustride012(1, 2, 3);
    assert!(unsafe { user_stride_equal(&base, &ustride012(1, 2, 3)) });
    assert!(!unsafe { user_stride_equal(&base, &ustride012(9, 2, 3)) });
    assert!(!unsafe { user_stride_equal(&base, &ustride012(1, 9, 3)) });
    assert!(!unsafe { user_stride_equal(&base, &ustride012(1, 2, 9)) });
    assert!(!unsafe { user_stride_equal(&base, &us(1, 2, 3, COLON_PRESENT)) });
    assert!(!unsafe { user_stride_equal(&base, ptr::null()) });
}

#[test]
fn malformed_strides_do_not_lift() {
    // Unknown flag, no colon and no start, stop without a colon, zero step
    for bad in [
        us(0, 0, 0, 1 << 7),
        us(0, 0, 0, 0),
        us(0, 1, 0, START_PRESENT | STOP_PRESENT),
        us(0, 0, 0, COLON_PRESENT | STEP_PRESENT),
    ] {
        assert_eq!(lift_stride(&bad), Err(Error::InvalidArgument), "{bad:?}");
    }
}

/// Port of TEST (compile_multi_user_stride)
#[test]
fn multi_user_strides() {
    let a = TestArena::new();
    let compile = |src: &str| {
        let mut dest: CMultiUserStride = zeroed();
        let mut e = NslangError::new();
        let src = cstr(src);
        let ret = unsafe {
            nslang_compile_multi_user_stride(&mut dest, src.as_ptr(), &a.alloc(), &mut e)
        };
        if ret == NSLANG_OK {
            let strides = if dest.len == 0 {
                assert!(dest.strides.is_null());
                Vec::new()
            } else {
                unsafe { slice::from_raw_parts(dest.strides, dest.len as usize) }.to_vec()
            };
            Ok(strides)
        } else {
            Err(ret)
        }
    };

    for src in ["[]", "[ ]", " [ ]", " [] "] {
        assert_eq!(compile(src), Ok(vec![]), "{src}");
    }

    assert_eq!(compile("[0]"), Ok(vec![ustride_single(0)]));
    assert_eq!(
        compile("[0, 0]"),
        Ok(vec![ustride_single(0), ustride_single(0)])
    );
    assert_eq!(compile("[:, 0]"), Ok(vec![ustride(), ustride_single(0)]));
    assert_eq!(
        compile("[0:0:1, 0]"),
        Ok(vec![ustride012(0, 0, 1), ustride_single(0)])
    );
    assert_eq!(
        compile("[1:2:3, 4:5:6, 7:8:9]"),
        Ok(vec![
            ustride012(1, 2, 3),
            ustride012(4, 5, 6),
            ustride012(7, 8, 9)
        ])
    );

    for src in [
        "0, 1]", "[0, 1", "", "[0,]", "[,0]", "[0,,1]", "[abc]", "42",
    ] {
        assert_eq!(compile(src), Err(NSLANG_ERR_SYNTAX), "{src}");
    }
}

// =============================================================================
// Types
// =============================================================================

/// Port of TEST (compile_type)
#[test]
fn compile_type_fills_in_the_c_structs() {
    let a = TestArena::new();

    for &p in Prim::ALL {
        let t = compile_type(&a, p.as_str());
        assert_eq!(t.tag, T_PRIM);
        assert_eq!(unsafe { t.u.p }, u32::from(p.code()));
    }

    let t = compile_type(&a, "struct { a i32, b union { c u8, d [10][20]f32 } }");
    assert_eq!(t.tag, T_STRUCT);
    let st = unsafe { t.u.fields };
    assert_eq!(st.len, 2);
    let keys = unsafe { slice::from_raw_parts(st.keys, 2) };
    assert_eq!(str_of(&keys[0]), "a");
    assert_eq!(str_of(&keys[1]), "b");
    // Keys are null terminated copies, though the terminator is not counted
    assert_eq!(unsafe { *keys[0].data.add(1) }, 0);

    let types = unsafe { slice::from_raw_parts(st.types, 2) };
    let (ta, tb) = unsafe { (&*types[0], &*types[1]) };
    assert_eq!(ta.tag, T_PRIM);
    assert_eq!(unsafe { ta.u.p }, u32::from(Prim::I32.code()));

    assert_eq!(tb.tag, T_UNION);
    let un = unsafe { tb.u.fields };
    assert_eq!(un.len, 2);
    let td = unsafe { &**un.types.add(1) };
    assert_eq!(td.tag, T_SARRAY);
    let sa = unsafe { td.u.sa };
    assert_eq!(sa.rank, 2);
    assert_eq!(unsafe { slice::from_raw_parts(sa.dims, 2) }, [10, 20]);
    assert_eq!(unsafe { (*sa.t).u.p }, u32::from(Prim::F32.code()));

    assert_eq!(unsafe { type_byte_size(&t) }, 4 + 10 * 20 * 4);
    assert_eq!(
        snprintf(&t),
        "struct { a i32, b union { c u8, d [10][20]f32 } }"
    );
}

#[test]
fn compile_errors_are_reported() {
    let a = TestArena::new();
    let mut dest: CType = zeroed();

    for src in [
        "",
        "struct",
        "struct { }",
        "struct { x i32, }",
        "[0]i32",
        "[10]",
        "i32 i32",
        "struct { x i32, x f64 }",
        "@",
    ] {
        let mut e = NslangError::new();
        let text = cstr(src);
        let ret = unsafe { nslang_compile_type(&mut dest, text.as_ptr(), &a.alloc(), &mut e) };
        assert_eq!(ret, NSLANG_ERR_SYNTAX, "{src}");
        assert_eq!(e.code, NSLANG_ERR_SYNTAX);
        assert_eq!(e.len as usize, e.message().len());
        assert!(e.message().starts_with("error: "), "{}", e.message());
        assert_eq!(e.msg[e.len as usize], 0);

        // The error is optional
        let ret =
            unsafe { nslang_compile_type(&mut dest, text.as_ptr(), &a.alloc(), ptr::null_mut()) };
        assert_eq!(ret, NSLANG_ERR_SYNTAX);
    }

    // A failed compile allocates nothing
    assert_eq!(a.allocations(), 0);
}

#[test]
fn long_error_messages_are_cut_to_fit() {
    let a = TestArena::new();
    let mut dest: CQuery = zeroed();
    let mut e = NslangError::new();
    let src = cstr(&"héllo ".repeat(100));
    let ret = unsafe { nslang_compile_query(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_ERR_SYNTAX);
    assert!(e.len <= 255);
    assert_eq!(e.msg[e.len as usize], 0);
}

#[test]
fn bad_arguments_are_errors_not_crashes() {
    let a = TestArena::new();
    let mut dest: CType = zeroed();
    let mut e = NslangError::new();
    let src = cstr("u8");

    let null_text = unsafe { nslang_compile_type(&mut dest, ptr::null(), &a.alloc(), &mut e) };
    assert_eq!(null_text, NSLANG_ERR_INVALID_ARGUMENT);

    let null_alloc = unsafe { nslang_compile_type(&mut dest, src.as_ptr(), ptr::null(), &mut e) };
    assert_eq!(null_alloc, NSLANG_ERR_INVALID_ARGUMENT);

    let null_dest =
        unsafe { nslang_compile_type(ptr::null_mut(), src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(null_dest, NSLANG_ERR_INVALID_ARGUMENT);

    let not_utf8 = [0xffu8, 0xfe, 0];
    let ret =
        unsafe { nslang_compile_type(&mut dest, not_utf8.as_ptr().cast(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_ERR_INVALID_ARGUMENT);

    assert_eq!(unsafe { type_byte_size(ptr::null()) }, 0);
    assert_eq!(
        unsafe { type_snprintf(ptr::null_mut(), 0, ptr::null()) },
        -1
    );
    assert!(!unsafe { type_equal(&prim(Prim::U8), ptr::null()) });
}

/// Port of TEST (type_validate_rejects)
#[test]
fn validate_rejects() {
    let validate = |t: &CType| {
        let mut e = NslangError::new();
        let ret = unsafe { nslang_type_validate(t, &mut e) };
        assert_eq!(ret, e.code);
        ret
    };

    let mut u8t = prim(Prim::U8);
    let mut u16t = prim(Prim::U16);
    assert_eq!(validate(&u8t), NSLANG_OK);

    // Unknown type code
    let mut t: CType = zeroed();
    t.tag = 99;
    assert_eq!(validate(&t), NSLANG_ERR_CORRUPT);

    // Bad prim
    let mut t: CType = zeroed();
    t.tag = T_PRIM;
    t.u.p = u32::from(Prim::CU128.code()) + 1;
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);

    // Bad struct: duplicate keys
    let mut keys = [s("a"), s("a")];
    let mut types = [&mut u8t as *mut CType, &mut u16t as *mut CType];
    let mut t: CType = zeroed();
    t.tag = T_STRUCT;
    t.u.fields = CFields {
        len: 2,
        keys: keys.as_mut_ptr(),
        types: types.as_mut_ptr(),
    };
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);

    // The same struct with distinct keys is fine
    unsafe { *t.u.fields.keys.add(1) = s("b") };
    assert_eq!(validate(&t), NSLANG_OK);

    // Bad union: zero length
    t.tag = T_UNION;
    t.u.fields.len = 0;
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);

    // Bad sarray: zero rank, then a zero dimension
    let mut dims = [4u32, 0];
    let mut t: CType = zeroed();
    t.tag = T_SARRAY;
    t.u.sa = CSArray {
        rank: 0,
        dims: dims.as_mut_ptr(),
        t: &mut u8t,
    };
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);
    t.u.sa.rank = 1;
    assert_eq!(validate(&t), NSLANG_OK);
    t.u.sa.rank = 2;
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);

    // Missing pieces
    t.u.sa.t = ptr::null_mut();
    assert_eq!(validate(&t), NSLANG_ERR_INVALID_TYPE);

    let mut e = NslangError::new();
    assert_eq!(
        unsafe { nslang_type_validate(ptr::null(), &mut e) },
        NSLANG_ERR_INVALID_TYPE
    );
}

/// Port of TEST (struct_t_equal), (union_t_equal), (sarray_t_equal)
#[test]
fn type_equality() {
    let a = TestArena::new();
    let srcs = [
        "u8",
        "u16",
        "struct { a u8 }",
        "struct { b u8 }",
        "struct { a u16 }",
        "struct { a u8, b u8 }",
        "union { a u8 }",
        "[1][2]u8",
        "[2][1]u8",
        "[1]u8",
        "[1][2]u16",
    ];
    let types: Vec<CType> = srcs.iter().map(|src| compile_type(&a, src)).collect();
    let again: Vec<CType> = srcs.iter().map(|src| compile_type(&a, src)).collect();

    for (i, left) in types.iter().enumerate() {
        for (j, right) in again.iter().enumerate() {
            assert_eq!(
                unsafe { type_equal(left, right) },
                i == j,
                "{} == {}",
                srcs[i],
                srcs[j]
            );
        }
    }
}

/// Port of TEST (prim_t_snprintf), (struct_t_snprintf), (sarray_t_snprintf)
#[test]
fn snprintf_follows_the_c_contract() {
    let a = TestArena::new();
    let t = compile_type(&a, "[10]struct { foo u32 }");
    let full = "[10]struct { foo u32 }";

    // NULL and size 0 only measure
    assert_eq!(
        unsafe { type_snprintf(ptr::null_mut(), 0, &t) },
        full.len() as i32
    );

    // A short buffer is truncated but returns the full length
    let mut buf = [b'#'; 8];
    let len = unsafe { type_snprintf(buf.as_mut_ptr().cast(), 8, &t) };
    assert_eq!(len, full.len() as i32);
    assert_eq!(&buf, b"[10]str\0");

    // A size of one has room for the terminator only
    let mut buf = [b'#'; 2];
    unsafe { type_snprintf(buf.as_mut_ptr().cast(), 1, &t) };
    assert_eq!(&buf, b"\0#");

    assert_eq!(snprintf(&t), full);
}

#[test]
fn tostr_and_copy() {
    let a = TestArena::new();
    let b = TestArena::new();
    let src = "struct { a [3]u16, b union { c f64, d i8 } }";
    let t = compile_type(&a, src);
    let mut e = NslangError::new();

    let text = unsafe { nslang_type_tostr(&t, &b.alloc(), &mut e) };
    assert!(!text.is_null());
    assert_eq!(unsafe { CStr::from_ptr(text) }.to_str().unwrap(), src);

    // A copy is equal, lives in the other arena, and shares nothing
    let copy = unsafe { nslang_type_copy(&t, &b.alloc(), &mut e) };
    assert!(!copy.is_null());
    assert!(unsafe { type_equal(&t, copy) });
    let (orig, dup) = unsafe { (t.u.fields, (*copy).u.fields) };
    assert_ne!(orig.keys, dup.keys);
    assert_ne!(orig.types, dup.types);
    assert_ne!(unsafe { (*orig.keys).data }, unsafe { (*dup.keys).data });

    drop(a);
    assert_eq!(snprintf(unsafe { &*copy }), src);
}

/// Port of TEST (type_movemem_out_of_memory), (type_tostr_out_of_memory),
/// (compile_type_fail_malloc), (lexer_fail_malloc), (*_deserialize_fail_malloc)
///
/// Fails the allocator at every allocation of every operation
#[test]
fn out_of_memory_at_every_allocation() {
    let good = TestArena::new();
    let src = "struct { a [3]u16, b union { c f64, d struct { e i8 } } }";
    let t = compile_type(&good, src);
    let text = cstr(src);
    let query = cstr("create foo struct { a [3]u16, b union { c f64 } }");
    let subtype = cstr("foo.a[0, 1:2].b[3]");
    let type_ref = cstr("struct { a foo.b[0], b struct { c foo.a } }");
    let strides = cstr("[1, 2:3, :]");
    let accessor = compile_subtype(&good, "foo.b.d");

    let mut bytes = vec![0u8; 256];
    let written = unsafe { nslang_type_serialize(bytes.as_mut_ptr(), 256, &t, ptr::null_mut()) };
    assert!(written > 0);

    type Op<'a> = Box<dyn Fn(&NslangAlloc, &mut NslangError) -> bool + 'a>;
    let ops: Vec<(&str, Op<'_>)> = vec![
        (
            "compile_type",
            Box::new(|a, e| unsafe {
                let mut dest: CType = zeroed();
                nslang_compile_type(&mut dest, text.as_ptr(), a, e) == NSLANG_OK
            }),
        ),
        (
            "compile_query",
            Box::new(|a, e| unsafe {
                let mut dest: CQuery = zeroed();
                nslang_compile_query(&mut dest, query.as_ptr(), a, e) == NSLANG_OK
            }),
        ),
        (
            "compile_subtype",
            Box::new(|a, e| unsafe {
                let mut dest: CSubtype = zeroed();
                nslang_compile_subtype(&mut dest, subtype.as_ptr(), a, e) == NSLANG_OK
            }),
        ),
        (
            "compile_type_ref",
            Box::new(|a, e| unsafe {
                let mut dest: CTypeRef = zeroed();
                nslang_compile_type_ref(&mut dest, type_ref.as_ptr(), a, e) == NSLANG_OK
            }),
        ),
        (
            "compile_multi_user_stride",
            Box::new(|a, e| unsafe {
                let mut dest: CMultiUserStride = zeroed();
                nslang_compile_multi_user_stride(&mut dest, strides.as_ptr(), a, e) == NSLANG_OK
            }),
        ),
        (
            "type_copy",
            Box::new(|a, e| unsafe { !nslang_type_copy(&t, a, e).is_null() }),
        ),
        (
            "type_tostr",
            Box::new(|a, e| unsafe { !nslang_type_tostr(&t, a, e).is_null() }),
        ),
        (
            "type_deserialize",
            Box::new(|a, e| unsafe {
                !nslang_type_deserialize(bytes.as_ptr(), written as u32, ptr::null_mut(), a, e)
                    .is_null()
            }),
        ),
        (
            "ta_subtype",
            Box::new(|a, e| unsafe { !nslang_ta_subtype(&t, &accessor.ta, a, e).is_null() }),
        ),
    ];

    for (name, op) in &ops {
        let arena = TestArena::new();
        let mut failures = 0;

        loop {
            arena.fail_after(failures);
            let mut e = NslangError::new();
            if op(&arena.alloc(), &mut e) {
                break;
            }
            assert_eq!(e.code, NSLANG_ERR_NOMEM, "{name} after {failures}");
            assert_eq!(e.message(), "out of memory");
            failures += 1;
            assert!(failures < 1000, "{name} never succeeds");
        }

        // Every operation here allocates at least once
        assert!(failures > 0, "{name}");
    }
}

// An allocator that ignores [align] is an error
#[test]
fn misaligned_allocations_are_refused() {
    let a = TestArena::new();
    a.misalign.set(true);

    let mut dest: CType = zeroed();
    let mut e = NslangError::new();
    let src = cstr("[4]u32");
    let ret = unsafe { nslang_compile_type(&mut dest, src.as_ptr(), &a.alloc(), &mut e) };
    assert_eq!(ret, NSLANG_ERR_NOMEM);
}

// =============================================================================
// Serialization
// =============================================================================

/// Port of TEST (struct_t_serialize), (sarray_t_serialize),
/// (*_deserialize_green_path)
#[test]
fn serialize_and_deserialize() {
    let a = TestArena::new();

    for src in [
        "u8",
        "cf256",
        "[10][11][12]u32",
        "struct { foo u32, fo u8, baro u16, bazbi cf128 }",
        "union { foo u32, fo u8, baro u16, bazbi cf128 }",
        "[2]struct { a union { b [3]i16, c f64 }, d u8 }",
    ] {
        let t = compile_type(&a, src);
        let mut e = NslangError::new();

        let mut size = 0u16;
        assert_eq!(
            unsafe { nslang_type_serial_size(&mut size, &t, &mut e) },
            NSLANG_OK
        );

        // One byte short does not fit, and writes nothing past the buffer
        let mut buf = vec![0xaau8; usize::from(size) + 8];
        let short =
            unsafe { nslang_type_serialize(buf.as_mut_ptr(), u32::from(size) - 1, &t, &mut e) };
        assert_eq!(short, NSLANG_ERR_ARITH, "{src}");
        assert!(buf.iter().all(|&b| b == 0xaa));

        // Exactly enough does
        let written = unsafe { nslang_type_serialize(buf.as_mut_ptr(), size.into(), &t, &mut e) };
        assert_eq!(written, i32::from(size), "{src}");
        assert!(buf[usize::from(size)..].iter().all(|&b| b == 0xaa));

        // And it reads back, using exactly the bytes that were written
        let mut consumed = 0u32;
        let back = unsafe {
            nslang_type_deserialize(
                buf.as_ptr(),
                buf.len() as u32,
                &mut consumed,
                &a.alloc(),
                &mut e,
            )
        };
        assert!(!back.is_null(), "{src}: {}", e.message());
        assert_eq!(consumed, u32::from(size));
        assert!(unsafe { type_equal(&t, back) });
        assert_eq!(snprintf(unsafe { &*back }), src);
    }
}

/// Port of TEST (type_deserialize_rejects) and the (*_early_termination) family
#[test]
fn deserialize_rejects() {
    let a = TestArena::new();
    let t = compile_type(&a, "struct { a [3]u16, b union { c f64, d i8 } }");
    let mut buf = [0u8; 128];
    let len = unsafe { nslang_type_serialize(buf.as_mut_ptr(), 128, &t, ptr::null_mut()) };
    assert!(len > 0);

    let before = a.allocations();

    for cut in 0..len as u32 {
        let mut e = NslangError::new();
        let mut consumed = 77u32;
        let got = unsafe {
            nslang_type_deserialize(buf.as_ptr(), cut, &mut consumed, &a.alloc(), &mut e)
        };
        assert!(got.is_null(), "cut at {cut}");
        assert_eq!(e.code, NSLANG_ERR_CORRUPT);
        assert_eq!(consumed, 77);
    }

    let mut e = NslangError::new();
    for bad in [&[99u8][..], &[0, 24], &[]] {
        let got = unsafe {
            nslang_type_deserialize(
                bad.as_ptr(),
                bad.len() as u32,
                ptr::null_mut(),
                &a.alloc(),
                &mut e,
            )
        };
        assert!(got.is_null());
        assert_eq!(e.code, NSLANG_ERR_CORRUPT);
    }
    let got =
        unsafe { nslang_type_deserialize(ptr::null(), 0, ptr::null_mut(), &a.alloc(), &mut e) };
    assert!(got.is_null());

    // Bytes are checked before anything is built
    assert_eq!(a.allocations(), before);
}

#[test]
fn invalid_types_do_not_serialize() {
    let mut t: CType = zeroed();
    t.tag = T_UNION;
    let mut buf = [0u8; 16];
    let mut e = NslangError::new();
    let mut size = 0u16;

    assert_eq!(
        unsafe { nslang_type_serialize(buf.as_mut_ptr(), 16, &t, &mut e) },
        NSLANG_ERR_INVALID_TYPE
    );
    t.tag = 99;
    assert_eq!(
        unsafe { nslang_type_serial_size(&mut size, &t, &mut e) },
        NSLANG_ERR_CORRUPT
    );
}

// =============================================================================
// Random types and printing
// =============================================================================

unsafe extern "C" fn test_range(ctx: *mut c_void, lo: u32, hi: u32) -> u32 {
    let state = unsafe { &mut *(ctx as *mut u64) };
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    lo + (*state % u64::from(hi - lo + 1)) as u32
}

unsafe extern "C" fn wild_range(_ctx: *mut c_void, _lo: u32, _hi: u32) -> u32 {
    u32::MAX
}

/// Port of TEST (type_random), (type_random_gives_up), (*_random_fail_malloc)
#[test]
fn random_types() {
    let a = TestArena::new();
    let mut state = 0x1234_5678u64;
    let rng = NslangRng {
        ctx: (&mut state as *mut u64).cast(),
        range: Some(test_range),
    };
    let mut e = NslangError::new();

    for _ in 0..100 {
        let t = unsafe { nslang_type_random(&a.alloc(), &rng, 3, 2048, &mut e) };
        assert!(!t.is_null(), "{}", e.message());
        assert_eq!(unsafe { nslang_type_validate(t, &mut e) }, NSLANG_OK);
        assert!(unsafe { type_byte_size(t) } <= 2048);
    }

    // Nothing is 0 bytes wide
    let none = unsafe { nslang_type_random(&a.alloc(), &rng, 2, 0, &mut e) };
    assert!(none.is_null());
    assert_eq!(e.code, NSLANG_ERR_INVALID_TYPE);

    // No memory
    a.fail_after(0);
    let none = unsafe { nslang_type_random(&a.alloc(), &rng, 0, 64, &mut e) };
    assert!(none.is_null());
    assert_eq!(e.code, NSLANG_ERR_NOMEM);
    a.unlimited();

    // A generator that ignores its bounds is clamped, not trusted
    let wild = NslangRng {
        ctx: ptr::null_mut(),
        range: Some(wild_range),
    };
    let t = unsafe { nslang_type_random(&a.alloc(), &wild, 2, u32::MAX, &mut e) };
    assert!(!t.is_null(), "{}", e.message());

    // No generator at all
    let none = unsafe { nslang_type_random(&a.alloc(), ptr::null(), 2, 64, &mut e) };
    assert!(none.is_null());
    assert_eq!(e.code, NSLANG_ERR_INVALID_ARGUMENT);
}

unsafe extern "C" fn collect(ctx: *mut c_void, text: *const c_char, len: usize) {
    let out = unsafe { &mut *(ctx as *mut Vec<u8>) };
    out.extend_from_slice(unsafe { slice::from_raw_parts(text.cast(), len) });
}

/// Port of TEST (type_print_data)
#[test]
fn print_data_writes_to_the_sink() {
    let a = TestArena::new();
    let mut out: Vec<u8> = Vec::new();
    let sink = NslangSink {
        ctx: (&mut out as *mut Vec<u8>).cast(),
        write: Some(collect),
    };

    let t = compile_type(&a, "struct { tag u32, values [3]i32 }");
    let mut buf = 7u32.to_ne_bytes().to_vec();
    for v in [1i32, -2, 3] {
        buf.extend(v.to_ne_bytes());
    }
    unsafe { nslang_type_print_data(buf.as_ptr(), &t, 2, &sink) };
    assert_eq!(
        String::from_utf8(out.clone()).unwrap(),
        "{\n    tag = 7,\n    values = [1, -2, ...]\n}\n"
    );

    // An empty union, as C can build one
    out.clear();
    let mut t: CType = zeroed();
    t.tag = T_UNION;
    unsafe { nslang_type_print_data(buf.as_ptr(), &t, 10, &sink) };
    assert_eq!(
        String::from_utf8(out.clone()).unwrap(),
        "<union[0]: empty>\n"
    );

    // A type that cannot be read says so
    out.clear();
    t.tag = 99;
    unsafe { nslang_type_print_data(buf.as_ptr(), &t, 10, &sink) };
    assert_eq!(String::from_utf8(out.clone()).unwrap(), "<invalid type>\n");

    // No sink, nothing to do
    unsafe { nslang_type_print_data(buf.as_ptr(), &t, 10, ptr::null()) };
}

// =============================================================================
// Type accessors, subtypes and type references
// =============================================================================

/// Port of TEST (compile_subtype) and (type_accessor_builder)
#[test]
fn compile_subtype_fills_in_the_c_structs() {
    let a = TestArena::new();

    let st = compile_subtype(&a, "myvar");
    assert_eq!(str_of(&st.vname), "myvar");
    assert_eq!(st.ta.tag, TA_TAKE);

    // myvar.a[0][1:2].b: select, one range of two strides, select, take
    let st = compile_subtype(&a, "myvar.a[0][1:2].b");
    assert_eq!(st.ta.tag, TA_SELECT);
    let sel = unsafe { st.ta.u.select };
    assert_eq!(str_of(&sel.key), "a");

    let range_ta = unsafe { &*sel.sub_ta };
    assert_eq!(range_ta.tag, TA_RANGE);
    let range = unsafe { range_ta.u.range };
    assert_eq!(range.dlen, 2);
    assert_eq!(
        unsafe { slice::from_raw_parts(range.dim_accessors, 2) },
        [
            ustride_single(0),
            us(1, 2, 0, COLON_PRESENT | START_PRESENT | STOP_PRESENT)
        ]
    );

    let sel = unsafe { &*range.sub_ta };
    assert_eq!(sel.tag, TA_SELECT);
    assert_eq!(str_of(unsafe { &sel.u.select.key }), "b");
    assert_eq!(unsafe { (*sel.u.select.sub_ta).tag }, TA_TAKE);

    let mut dest: CSubtype = zeroed();
    for src in ["", "42", "[0]", "struct a x }", "foo.", ".bar"] {
        let text = cstr(src);
        let ret = unsafe {
            nslang_compile_subtype(&mut dest, text.as_ptr(), &a.alloc(), ptr::null_mut())
        };
        assert_eq!(ret, NSLANG_ERR_SYNTAX, "{src}");
    }
}

/// Port of TEST (subtype_equal)
#[test]
fn subtype_and_accessor_equality() {
    let a = TestArena::new();
    let srcs = [
        "v", "w", "v.a", "v.b", "v.a.b", "v[0]", "v[1]", "v[0, 1]", "v[0:1]", "v[0].a", "v.a[0]",
    ];
    let left: Vec<CSubtype> = srcs.iter().map(|src| compile_subtype(&a, src)).collect();
    let right: Vec<CSubtype> = srcs.iter().map(|src| compile_subtype(&a, src)).collect();

    for (i, l) in left.iter().enumerate() {
        for (j, r) in right.iter().enumerate() {
            assert_eq!(
                unsafe { subtype_equal(l, r) },
                i == j,
                "{} == {}",
                srcs[i],
                srcs[j]
            );

            // The accessors alone differ unless only the variable name does
            let same_accessor = i == j || (i < 2 && j < 2);
            assert_eq!(
                unsafe { nslang_type_accessor_equal(&l.ta, &r.ta) },
                same_accessor,
                "{} ~ {}",
                srcs[i],
                srcs[j]
            );
        }
    }

    // A NULL sub_ta is a take
    let mut by_hand: CTypeAccessor = zeroed();
    by_hand.tag = TA_SELECT;
    by_hand.u.select = CSelect {
        key: s("a"),
        sub_ta: ptr::null_mut(),
    };
    assert!(unsafe { nslang_type_accessor_equal(&by_hand, &left[2].ta) });
}

/// Port of TEST (compile_type_ref) and (type_ref_equal)
#[test]
fn type_refs() {
    let a = TestArena::new();

    let tr = compile_type_ref(&a, "struct { a x, b struct { c y.z[0] } }");
    assert_eq!(tr.tag, TR_STRUCT);
    let st = unsafe { tr.u.st };
    assert_eq!(st.len, 2);
    assert_eq!(str_of(unsafe { &*st.keys }), "a");
    assert_eq!(str_of(unsafe { &*st.keys.add(1) }), "b");

    let first = unsafe { &*st.types };
    assert_eq!(first.tag, TR_TAKE);
    assert_eq!(str_of(unsafe { &first.u.tk.vname }), "x");
    assert_eq!(unsafe { first.u.tk.ta.tag }, TA_TAKE);

    let second = unsafe { &*st.types.add(1) };
    assert_eq!(second.tag, TR_STRUCT);
    let inner = unsafe { &*second.u.st.types };
    assert_eq!(str_of(unsafe { &inner.u.tk.vname }), "y");
    assert_eq!(unsafe { inner.u.tk.ta.tag }, TA_SELECT);

    let srcs = [
        "x",
        "y",
        "x.a",
        "x[0]",
        "struct { a x }",
        "struct { b x }",
        "struct { a y }",
        "struct { a x, b y }",
        "struct { a struct { b x } }",
    ];
    let left: Vec<CTypeRef> = srcs.iter().map(|src| compile_type_ref(&a, src)).collect();
    let right: Vec<CTypeRef> = srcs.iter().map(|src| compile_type_ref(&a, src)).collect();
    for (i, l) in left.iter().enumerate() {
        for (j, r) in right.iter().enumerate() {
            assert_eq!(
                unsafe { nslang_type_ref_equal(l, r) },
                i == j,
                "{} == {}",
                srcs[i],
                srcs[j]
            );
        }
    }

    let mut dest: CTypeRef = zeroed();
    for src in [
        "",
        "42",
        "[0]",
        "struct {}",
        "struct { a }",
        "struct { a x, }",
    ] {
        let text = cstr(src);
        let ret = unsafe {
            nslang_compile_type_ref(&mut dest, text.as_ptr(), &a.alloc(), ptr::null_mut())
        };
        assert_eq!(ret, NSLANG_ERR_SYNTAX, "{src}");
    }
}

/// Port of TEST (ta_subtype), (ta_subtype_rejects) and (tr_construct)
/// The full tables are in types/type_accessor.rs and types/type_ref.rs
#[test]
fn subtype_and_construct() {
    let a = TestArena::new();
    let mut e = NslangError::new();

    let reftype = compile_type(&a, "[10]struct { x f32, y [4]u8 }");
    let st = compile_subtype(&a, "v[0:10:2].y[1]");
    let sub = unsafe { nslang_ta_subtype(&reftype, &st.ta, &a.alloc(), &mut e) };
    assert!(!sub.is_null(), "{}", e.message());
    assert_eq!(snprintf(unsafe { &*sub }), "[5]u8");

    // A take is the whole type, as a copy
    let whole = compile_subtype(&a, "v");
    let sub = unsafe { nslang_ta_subtype(&reftype, &whole.ta, &a.alloc(), &mut e) };
    assert!(unsafe { type_equal(sub, &reftype) });
    assert_ne!(sub.cast_const(), &reftype as *const CType);

    let bad = compile_subtype(&a, "v.z");
    let sub = unsafe { nslang_ta_subtype(&reftype, &bad.ta, &a.alloc(), &mut e) };
    assert!(sub.is_null());
    assert_eq!(e.code, NSLANG_ERR_INVALID_TYPE);
    assert_eq!(e.message(), "No field named 'z'");

    let base = compile_type(&a, "struct { a u8, b struct { c u16 } }");
    let tr = compile_type_ref(&a, "struct { a foo.b, b foo.a }");
    let built = unsafe { nslang_tr_construct(&base, &tr, &a.alloc(), &mut e) };
    assert!(!built.is_null(), "{}", e.message());
    assert_eq!(
        snprintf(unsafe { &*built }),
        "struct { a struct { c u16 }, b u8 }"
    );

    let tr = compile_type_ref(&a, "foo.a.b");
    let built = unsafe { nslang_tr_construct(&base, &tr, &a.alloc(), &mut e) };
    assert!(built.is_null());
    assert_eq!(e.code, NSLANG_ERR_INVALID_TYPE);
}

// =============================================================================
// Queries
// =============================================================================

/// Port of TEST (compile_query)
#[test]
fn compile_query_fills_in_the_c_structs() {
    let a = TestArena::new();

    for (verb, tag) in [
        ("read", QT_READ),
        ("write", QT_WRITE),
        ("remove", QT_REMOVE),
    ] {
        let q = compile_query(&a, &format!("{verb} foo"));
        assert_eq!(q.tag, tag);
        assert_eq!(str_of(unsafe { &q.u.strided.name }), "foo");
        assert_eq!(unsafe { q.u.strided.ustr }, ustride());

        let q = compile_query(&a, &format!("{verb} foo[0:10:20]"));
        assert_eq!(unsafe { q.u.strided.ustr }, ustride012(0, 10, 20));

        let q = compile_query(&a, &format!("{verb} foo[7]"));
        assert_eq!(unsafe { q.u.strided.ustr }, ustride_single(7));
    }

    let q = compile_query(&a, "insert foo -3 12");
    assert_eq!(q.tag, QT_INSERT);
    let ins = unsafe { q.u.insert };
    assert_eq!((str_of(&ins.name), ins.ofst, ins.len), ("foo", -3, 12));

    let q = compile_query(
        &a,
        "create foo struct { a i32, b union { c i32, d [10][20]f32 } }",
    );
    assert_eq!(q.tag, QT_CREATE);
    assert_eq!(str_of(unsafe { &q.u.create.name }), "foo");
    assert_eq!(
        snprintf(unsafe { &q.u.create.ty }),
        "struct { a i32, b union { c i32, d [10][20]f32 } }"
    );

    for (verb, tag) in [("delete", QT_DELETE), ("get", QT_GET)] {
        let q = compile_query(&a, &format!("{verb} foo"));
        assert_eq!(q.tag, tag);
        assert_eq!(str_of(unsafe { &q.u.named.name }), "foo");
        assert_eq!(unsafe { q.u.named.if_exists }, 0);

        let q = compile_query(&a, &format!("{verb} if exists foo"));
        assert_eq!(unsafe { q.u.named.if_exists }, 1);
    }

    assert_eq!(compile_query(&a, "exit").tag, QT_EXIT);

    let q = compile_query(&a, "help");
    assert_eq!(q.tag, QT_HELP);
    assert_eq!(unsafe { q.u.help.has_command }, 0);
    for (word, tag) in [
        ("read", QT_READ),
        ("create", QT_CREATE),
        ("delete", QT_DELETE),
        ("get", QT_GET),
        ("exit", QT_EXIT),
        ("help", QT_HELP),
    ] {
        let q = compile_query(&a, &format!("help {word}"));
        assert_eq!(unsafe { q.u.help.has_command }, 1);
        assert_eq!(unsafe { q.u.help.command }, tag, "{word}");
    }

    // The name is a copy: it outlives the text it came from
    let text = String::from("get somewhat_long_name");
    let q = compile_query(&a, &text);
    drop(text);
    let name = unsafe { q.u.named.name };
    assert_eq!(str_of(&name), "somewhat_long_name");
    assert_eq!(unsafe { *name.data.add(name.len as usize) }, 0);

    // What follows a complete statement is dropped
    let q = compile_query(&a, "read foo[0:] blimit 8");
    assert_eq!(q.tag, QT_READ);
    assert_eq!(compile_query(&a, "create foo u32;").tag, QT_CREATE);

    let mut dest: CQuery = zeroed();
    for src in [
        "",
        "read",
        "read 10",
        "write",
        "remove 10",
        "insert foo",
        "insert foo 0",
        "insert foo 0 -1",
        "create",
        "create foo",
        "create foo 1",
        "delete 1",
        "get if",
        "get if foo",
        "foo",
        "struct",
    ] {
        let text = cstr(src);
        let ret =
            unsafe { nslang_compile_query(&mut dest, text.as_ptr(), &a.alloc(), ptr::null_mut()) };
        assert_eq!(ret, NSLANG_ERR_SYNTAX, "{src}");
    }
}

/// Port of TEST (query_equal)
#[test]
fn query_equality() {
    let a = TestArena::new();
    let srcs = [
        "read foo",
        "read bar",
        "read foo[1:2:3]",
        "write foo",
        "remove foo",
        "insert foo 0 1",
        "insert bar 0 1",
        "insert foo 5 1",
        "insert foo 0 5",
        "create foo u32",
        "create bar u32",
        "create foo u64",
        "delete foo",
        "delete bar",
        "delete if exists foo",
        "get foo",
        "get bar",
        "get if exists foo",
        "exit",
        "help",
        "help read",
        "help get",
    ];
    let left: Vec<CQuery> = srcs.iter().map(|src| compile_query(&a, src)).collect();
    let right: Vec<CQuery> = srcs.iter().map(|src| compile_query(&a, src)).collect();

    for (i, l) in left.iter().enumerate() {
        for (j, r) in right.iter().enumerate() {
            assert_eq!(
                unsafe { query_equal(l, r) },
                i == j,
                "{} == {}",
                srcs[i],
                srcs[j]
            );
        }
    }

    assert!(!unsafe { query_equal(&left[0], ptr::null()) });
}

/// Port of TEST (query_vname_of_interest)
#[test]
fn query_variable_name() {
    let a = TestArena::new();

    for src in [
        "read foo",
        "write foo[0:1]",
        "remove foo",
        "insert foo 0 1",
        "create foo u8",
        "delete foo",
        "get if exists foo",
    ] {
        let q = compile_query(&a, src);
        let mut name = s("unset");
        assert!(unsafe { query_vname(&mut name, &q) }, "{src}");
        assert_eq!(str_of(&name), "foo", "{src}");
    }

    // Exit and help name no variable, and leave dest alone
    for src in ["exit", "help", "help create"] {
        let q = compile_query(&a, src);
        let mut name = s("unset");
        assert!(!unsafe { query_vname(&mut name, &q) }, "{src}");
        assert_eq!(str_of(&name), "unset");
    }

    let q = compile_query(&a, "exit");
    assert!(!unsafe { query_vname(ptr::null_mut(), &q) });
}
