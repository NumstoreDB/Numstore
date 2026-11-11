// The c functions of nslang - declared in include/nslang.h
//
// lower_*: rust value -> c struct allocated on the caller's nslang_alloc
// lift_*:  c struct -> rust value that borrows from the c struct
//
// A panic can't unwind into c, so nothing here panics on bad input

#![allow(clippy::missing_safety_doc)]

use std::ffi::{CStr, c_char, c_void};
use std::fmt::Write as _;
use std::num::NonZeroI64;
use std::{mem, ptr, slice};

use crate::compiler;
use crate::error::{Error, Result};
use crate::types::print::print_data;
use crate::types::query::{Query, QueryKind};
use crate::types::random::random_type;
use crate::types::sub_type::Subtype;
use crate::types::type_accessor::TypeAccessor;
use crate::types::type_ref::{StructRef, TypeRef};
use crate::types::types::{Prim, SArray, Struct, Type, Union};
use crate::types::user_stride::UserStride;

// =============================================================================
// Callbacks
// =============================================================================

// struct nslang_alloc
#[repr(C)]
pub struct NslangAlloc {
    pub ctx: *mut c_void,
    pub alloc:
        Option<unsafe extern "C" fn(ctx: *mut c_void, size: usize, align: usize) -> *mut c_void>,
}

// struct nslang_rng
#[repr(C)]
pub struct NslangRng {
    pub ctx: *mut c_void,
    pub range: Option<unsafe extern "C" fn(ctx: *mut c_void, lo: u32, hi: u32) -> u32>,
}

// struct nslang_sink
#[repr(C)]
pub struct NslangSink {
    pub ctx: *mut c_void,
    pub write: Option<unsafe extern "C" fn(ctx: *mut c_void, text: *const c_char, len: usize)>,
}

// =============================================================================
// Errors
// =============================================================================

pub const NSLANG_OK: i32 = 0;
pub const NSLANG_ERR_NOMEM: i32 = -1;
pub const NSLANG_ERR_SYNTAX: i32 = -2;
pub const NSLANG_ERR_INVALID_TYPE: i32 = -3;
pub const NSLANG_ERR_CORRUPT: i32 = -4;
pub const NSLANG_ERR_ARITH: i32 = -5;
pub const NSLANG_ERR_INVALID_ARGUMENT: i32 = -6;

// struct nslang_error
#[repr(C)]
pub struct NslangError {
    pub code: i32,
    pub len: u32,
    pub msg: [c_char; 256],
}

impl NslangError {
    pub const fn new() -> Self {
        NslangError {
            code: NSLANG_OK,
            len: 0,
            msg: [0; 256],
        }
    }

    pub fn message(&self) -> String {
        let bytes: Vec<u8> = self.msg[..(self.len as usize).min(255)]
            .iter()
            .map(|&c| c as u8)
            .collect();
        String::from_utf8_lossy(&bytes).into_owned()
    }
}

impl Default for NslangError {
    fn default() -> Self {
        Self::new()
    }
}

fn code_of(err: &Error) -> i32 {
    match err {
        Error::Lex(_) | Error::Parse(_) => NSLANG_ERR_SYNTAX,
        Error::InvalidType(_) => NSLANG_ERR_INVALID_TYPE,
        Error::Corrupt(_) => NSLANG_ERR_CORRUPT,
        Error::Arith(_) => NSLANG_ERR_ARITH,
        Error::NoMem => NSLANG_ERR_NOMEM,
        Error::InvalidArgument => NSLANG_ERR_INVALID_ARGUMENT,
    }
}

// Writes [err] to [e] and returns its code
unsafe fn report(e: *mut NslangError, err: &Error) -> i32 {
    let code = code_of(err);

    if let Some(e) = unsafe { e.as_mut() } {
        let mut text = String::new();
        let _ = write!(text, "{err}");

        // Truncate on a char boundary
        let mut len = text.len().min(e.msg.len() - 1);
        while !text.is_char_boundary(len) {
            len -= 1;
        }

        for (dest, &src) in e.msg.iter_mut().zip(&text.as_bytes()[..len]) {
            *dest = src as c_char;
        }
        e.msg[len] = 0;
        e.len = len as u32;
        e.code = code;
    }

    code
}

unsafe fn status(e: *mut NslangError, f: impl FnOnce() -> Result<()>) -> i32 {
    match f() {
        Ok(()) => NSLANG_OK,
        Err(err) => unsafe { report(e, &err) },
    }
}

unsafe fn pointer<T>(e: *mut NslangError, f: impl FnOnce() -> Result<*mut T>) -> *mut T {
    match f() {
        Ok(p) => p,
        Err(err) => {
            unsafe { report(e, &err) };
            ptr::null_mut()
        }
    }
}

// =============================================================================
// C structs - these mirror the numstore headers
// =============================================================================

pub const T_PRIM: u32 = 0;
pub const T_STRUCT: u32 = 1;
pub const T_UNION: u32 = 2;
pub const T_SARRAY: u32 = 3;

pub const TA_TAKE: u32 = 0;
pub const TA_SELECT: u32 = 1;
pub const TA_RANGE: u32 = 2;

pub const TR_TAKE: u32 = 0;
pub const TR_STRUCT: u32 = 1;

// struct string (core/ns_string.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CStr32 {
    pub len: u32,
    pub data: *const c_char,
}

// struct type (nscore/types/ns_types.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CType {
    pub tag: u32,
    pub u: CTypeBody,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union CTypeBody {
    pub p: u32,
    // st and un
    pub fields: CFields,
    pub sa: CSArray,
}

// struct struct_t and struct union_t (nscore/types/ns_types.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CFields {
    pub len: u16,
    pub keys: *mut CStr32,
    pub types: *mut *mut CType,
}

// struct sarray_t (nscore/types/ns_types.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CSArray {
    pub rank: u16,
    pub dims: *mut u32,
    pub t: *mut CType,
}

// struct type_accessor (nscore/types/ns_type_accessor.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CTypeAccessor {
    pub tag: u32,
    pub u: CTypeAccessorBody,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union CTypeAccessorBody {
    pub select: CSelect,
    pub range: CRange,
}

// struct select_ta (nscore/types/ns_type_accessor.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CSelect {
    pub key: CStr32,
    pub sub_ta: *mut CTypeAccessor,
}

// struct range_ta (nscore/types/ns_type_accessor.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CRange {
    pub dim_accessors: *mut CUserStride,
    pub dlen: u32,
    pub sub_ta: *mut CTypeAccessor,
}

// struct subtype (nscore/types/ns_subtype.h) and struct take_tr
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CSubtype {
    pub vname: CStr32,
    pub ta: CTypeAccessor,
}

// struct type_ref (nscore/types/ns_type_ref.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CTypeRef {
    pub tag: u32,
    pub u: CTypeRefBody,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union CTypeRefBody {
    pub tk: CSubtype,
    pub st: CStructRef,
}

// struct struct_tr (nscore/types/ns_type_ref.h)
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CStructRef {
    pub len: u16,
    pub keys: *mut CStr32,
    pub types: *mut CTypeRef,
}

// =============================================================================
// Lowering
// =============================================================================

pub struct Arena<'a>(&'a NslangAlloc);

impl<'a> Arena<'a> {
    unsafe fn new(alloc: *const NslangAlloc) -> Result<Arena<'a>> {
        unsafe { alloc.as_ref() }
            .filter(|a| a.alloc.is_some())
            .map(Arena)
            .ok_or(Error::InvalidArgument)
    }
}

fn count_u16(n: usize, what: &str) -> Result<u16> {
    u16::try_from(n).map_err(|_| Error::InvalidType(format!("too many {what}: {n}")))
}

fn lower_type(a: &Arena<'_>, t: &Type<'_>) -> Result<CType> {
    // Zero the whole union - c might look at any byte of it
    let mut out: CType = unsafe { mem::zeroed() };

    match t {
        Type::Prim(p) => {
            out.tag = T_PRIM;
            out.u.p = p.code().into();
        }
        Type::Struct(Struct { keys, types }) | Type::Union(Union { keys, types }) => {
            let subs = a.array::<*mut CType>(types.len())?;
            for (i, sub) in types.iter().enumerate() {
                unsafe { subs.add(i).write(a.put(lower_type(a, sub)?)?) };
            }
            out.tag = if matches!(t, Type::Struct(_)) {
                T_STRUCT
            } else {
                T_UNION
            };
            out.u.fields = CFields {
                len: count_u16(keys.len(), "fields")?,
                keys: a.strings(keys)?,
                types: subs,
            };
        }
        Type::SArray(sa) => {
            out.tag = T_SARRAY;
            out.u.sa = CSArray {
                rank: count_u16(sa.dims.len(), "dimensions")?,
                dims: a.slice(&sa.dims)?,
                t: a.put(lower_type(a, &sa.t)?)?,
            };
        }
    }

    Ok(out)
}

fn lower_accessor(a: &Arena<'_>, ta: &TypeAccessor<'_>) -> Result<CTypeAccessor> {
    let mut out: CTypeAccessor = unsafe { mem::zeroed() };

    match ta {
        TypeAccessor::Take => out.tag = TA_TAKE,
        TypeAccessor::Select { key, sub } => {
            out.tag = TA_SELECT;
            out.u.select = CSelect {
                key: a.string(key)?,
                sub_ta: a.put(lower_accessor(a, sub)?)?,
            };
        }
        TypeAccessor::Range { strides, sub } => {
            out.tag = TA_RANGE;
            out.u.range = CRange {
                dim_accessors: lower_strides(a, strides)?,
                dlen: u32::try_from(strides.len()).map_err(|_| Error::NoMem)?,
                sub_ta: a.put(lower_accessor(a, sub)?)?,
            };
        }
    }

    Ok(out)
}

fn lower_subtype(a: &Arena<'_>, st: &Subtype<'_>) -> Result<CSubtype> {
    Ok(CSubtype {
        vname: a.string(st.vname)?,
        ta: lower_accessor(a, &st.ta)?,
    })
}

fn lower_type_ref(a: &Arena<'_>, tr: &TypeRef<'_>) -> Result<CTypeRef> {
    let mut out: CTypeRef = unsafe { mem::zeroed() };

    match tr {
        TypeRef::Take(st) => {
            out.tag = TR_TAKE;
            out.u.tk = lower_subtype(a, st)?;
        }
        TypeRef::Struct(s) => {
            let types = a.array::<CTypeRef>(s.types.len())?;
            for (i, sub) in s.types.iter().enumerate() {
                unsafe { types.add(i).write(lower_type_ref(a, sub)?) };
            }
            out.tag = TR_STRUCT;
            out.u.st = CStructRef {
                len: count_u16(s.keys.len(), "fields")?,
                keys: a.strings(&s.keys)?,
                types,
            };
        }
    }

    Ok(out)
}

// =============================================================================
// Lifting
// =============================================================================

fn invalid(msg: &str) -> Error {
    Error::InvalidType(msg.into())
}

unsafe fn bytes_of<'a>(s: &CStr32) -> &'a [u8] {
    if s.len == 0 || s.data.is_null() {
        &[]
    } else {
        unsafe { slice::from_raw_parts(s.data.cast(), s.len as usize) }
    }
}

unsafe fn lift_str<'a>(s: &CStr32) -> Result<&'a str> {
    if s.len > 0 && s.data.is_null() {
        return Err(invalid("string has a length but no data"));
    }
    std::str::from_utf8(unsafe { bytes_of(s) }).map_err(|_| invalid("string is not valid UTF-8"))
}

unsafe fn lift_strs<'a>(keys: *const CStr32, len: usize) -> Result<Box<[&'a str]>> {
    if len > 0 && keys.is_null() {
        return Err(invalid("missing keys"));
    }
    (0..len)
        .map(|i| unsafe { lift_str(&*keys.add(i)) })
        .collect()
}

unsafe fn lift_type<'a>(t: *const CType) -> Result<Type<'a>> {
    let Some(t) = (unsafe { t.as_ref() }) else {
        return Err(invalid("missing type"));
    };

    match t.tag {
        T_PRIM => {
            let code = unsafe { t.u.p };
            Prim::from_code(code)
                .map(Type::Prim)
                .ok_or_else(|| Error::InvalidType(format!("invalid prim type {code}")))
        }
        T_STRUCT | T_UNION => {
            let f = unsafe { t.u.fields };
            let len = usize::from(f.len);
            if len > 0 && f.types.is_null() {
                return Err(invalid("missing field types"));
            }
            let keys = unsafe { lift_strs(f.keys, len) }?;
            let types = (0..len)
                .map(|i| unsafe { lift_type(*f.types.add(i)) })
                .collect::<Result<Box<[_]>>>()?;
            Ok(if t.tag == T_STRUCT {
                Type::Struct(Struct { keys, types })
            } else {
                Type::Union(Union { keys, types })
            })
        }
        T_SARRAY => {
            let sa = unsafe { t.u.sa };
            let rank = usize::from(sa.rank);
            if rank > 0 && sa.dims.is_null() {
                return Err(invalid("missing dimensions"));
            }
            let dims: Box<[u32]> = if rank == 0 {
                Box::new([])
            } else {
                unsafe { slice::from_raw_parts(sa.dims, rank) }.into()
            };
            Ok(Type::SArray(SArray {
                dims,
                t: Box::new(unsafe { lift_type(sa.t) }?),
            }))
        }
        other => Err(Error::Corrupt(format!("Unknown type code: {other}"))),
    }
}

fn lift_stride(s: &CUserStride) -> Result<UserStride> {
    let all = START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT;
    if s.present & !all != 0 {
        return Err(Error::InvalidArgument);
    }

    // No colon - a bare index: [start]
    if s.present & COLON_PRESENT == 0 {
        return if s.present == START_PRESENT {
            Ok(UserStride::Index(s.start))
        } else {
            Err(Error::InvalidArgument)
        };
    }

    let step = if s.present & STEP_PRESENT != 0 {
        Some(NonZeroI64::new(s.step).ok_or(Error::InvalidArgument)?)
    } else {
        None
    };

    Ok(UserStride::Slice {
        start: (s.present & START_PRESENT != 0).then_some(s.start),
        stop: (s.present & STOP_PRESENT != 0).then_some(s.stop),
        step,
    })
}

unsafe fn lift_accessor<'a>(ta: *const CTypeAccessor) -> Result<TypeAccessor<'a>> {
    // NULL is a take
    let Some(ta) = (unsafe { ta.as_ref() }) else {
        return Ok(TypeAccessor::Take);
    };

    match ta.tag {
        TA_TAKE => Ok(TypeAccessor::Take),
        TA_SELECT => {
            let s = unsafe { ta.u.select };
            Ok(TypeAccessor::Select {
                key: unsafe { lift_str(&s.key) }?,
                sub: Box::new(unsafe { lift_accessor(s.sub_ta) }?),
            })
        }
        TA_RANGE => {
            let r = unsafe { ta.u.range };
            let len = r.dlen as usize;
            if len > 0 && r.dim_accessors.is_null() {
                return Err(invalid("missing dimension accessors"));
            }
            let strides = (0..len)
                .map(|i| lift_stride(unsafe { &*r.dim_accessors.add(i) }))
                .collect::<Result<Box<[_]>>>()?;
            Ok(TypeAccessor::Range {
                strides,
                sub: Box::new(unsafe { lift_accessor(r.sub_ta) }?),
            })
        }
        _ => Err(invalid("unknown type accessor")),
    }
}

unsafe fn lift_type_ref<'a>(tr: *const CTypeRef) -> Result<TypeRef<'a>> {
    let Some(tr) = (unsafe { tr.as_ref() }) else {
        return Err(invalid("missing type ref"));
    };

    match tr.tag {
        TR_TAKE => {
            let tk = unsafe { tr.u.tk };
            Ok(TypeRef::Take(Subtype {
                vname: unsafe { lift_str(&tk.vname) }?,
                ta: unsafe { lift_accessor(&tk.ta) }?,
            }))
        }
        TR_STRUCT => {
            let st = unsafe { tr.u.st };
            let len = usize::from(st.len);
            if len > 0 && st.types.is_null() {
                return Err(invalid("missing field type refs"));
            }
            Ok(TypeRef::Struct(StructRef {
                keys: unsafe { lift_strs(st.keys, len) }?,
                types: (0..len)
                    .map(|i| unsafe { lift_type_ref(st.types.add(i)) })
                    .collect::<Result<Box<[_]>>>()?,
            }))
        }
        _ => Err(invalid("unknown type ref")),
    }
}

unsafe fn text<'a>(p: *const c_char) -> Result<&'a str> {
    if p.is_null() {
        return Err(Error::InvalidArgument);
    }
    unsafe { CStr::from_ptr(p) }
        .to_str()
        .map_err(|_| Error::InvalidArgument)
}

// [len] bytes of [p] - not null terminated
unsafe fn word<'a>(p: *const c_char, len: u32) -> Option<&'a str> {
    if p.is_null() {
        return None;
    }
    std::str::from_utf8(unsafe { slice::from_raw_parts(p.cast(), len as usize) }).ok()
}

// =============================================================================
// Equality and size
// These don't lift - they get called a lot and must not allocate
// =============================================================================

unsafe fn str_eq(left: &CStr32, right: &CStr32) -> bool {
    left.len == right.len && unsafe { bytes_of(left) == bytes_of(right) }
}

unsafe fn fields_eq(left: &CFields, right: &CFields) -> bool {
    if left.len != right.len {
        return false;
    }
    if left.len > 0
        && (left.keys.is_null()
            || right.keys.is_null()
            || left.types.is_null()
            || right.types.is_null())
    {
        return false;
    }
    (0..usize::from(left.len)).all(|i| unsafe {
        str_eq(&*left.keys.add(i), &*right.keys.add(i))
            && type_eq(*left.types.add(i), *right.types.add(i))
    })
}

unsafe fn type_eq(left: *const CType, right: *const CType) -> bool {
    let (Some(l), Some(r)) = (unsafe { left.as_ref() }, unsafe { right.as_ref() }) else {
        return left.is_null() && right.is_null();
    };
    if l.tag != r.tag {
        return false;
    }

    match l.tag {
        T_PRIM => unsafe { l.u.p == r.u.p },
        T_STRUCT | T_UNION => unsafe { fields_eq(&l.u.fields, &r.u.fields) },
        T_SARRAY => {
            let (l, r) = unsafe { (l.u.sa, r.u.sa) };
            if l.rank != r.rank {
                return false;
            }
            if l.rank > 0 && (l.dims.is_null() || r.dims.is_null()) {
                return false;
            }
            (0..usize::from(l.rank)).all(|i| unsafe { *l.dims.add(i) == *r.dims.add(i) })
                && unsafe { type_eq(l.t, r.t) }
        }
        _ => false,
    }
}

unsafe fn byte_size(t: *const CType) -> u32 {
    let Some(t) = (unsafe { t.as_ref() }) else {
        return 0;
    };

    match t.tag {
        T_PRIM => Prim::from_code(unsafe { t.u.p }).map_or(0, |p| p.size_bytes() as u32),
        T_STRUCT | T_UNION => {
            let f = unsafe { t.u.fields };
            if f.types.is_null() {
                return 0;
            }
            let sizes = (0..usize::from(f.len)).map(|i| unsafe { byte_size(*f.types.add(i)) });
            if t.tag == T_STRUCT {
                sizes.fold(0u32, u32::wrapping_add)
            } else {
                sizes.max().unwrap_or(0)
            }
        }
        T_SARRAY => {
            let sa = unsafe { t.u.sa };
            if sa.dims.is_null() {
                return 0;
            }
            (0..usize::from(sa.rank))
                .fold(1u32, |acc, i| acc.wrapping_mul(unsafe { *sa.dims.add(i) }))
                .wrapping_mul(unsafe { byte_size(sa.t) })
        }
        _ => 0,
    }
}

unsafe fn accessor_eq(left: *const CTypeAccessor, right: *const CTypeAccessor) -> bool {
    // NULL is a take
    let tag = |ta: *const CTypeAccessor| unsafe { ta.as_ref() }.map_or(TA_TAKE, |ta| ta.tag);
    if tag(left) != tag(right) {
        return false;
    }

    match tag(left) {
        TA_TAKE => true,
        TA_SELECT => {
            let (l, r) = unsafe { ((*left).u.select, (*right).u.select) };
            unsafe { str_eq(&l.key, &r.key) && accessor_eq(l.sub_ta, r.sub_ta) }
        }
        TA_RANGE => {
            let (l, r) = unsafe { ((*left).u.range, (*right).u.range) };
            if l.dlen != r.dlen {
                return false;
            }
            if l.dlen > 0 && (l.dim_accessors.is_null() || r.dim_accessors.is_null()) {
                return false;
            }
            (0..l.dlen as usize)
                .all(|i| unsafe { *l.dim_accessors.add(i) == *r.dim_accessors.add(i) })
                && unsafe { accessor_eq(l.sub_ta, r.sub_ta) }
        }
        _ => false,
    }
}

unsafe fn type_ref_eq(left: &CTypeRef, right: &CTypeRef) -> bool {
    if left.tag != right.tag {
        return false;
    }

    match left.tag {
        TR_TAKE => {
            let (l, r) = unsafe { (left.u.tk, right.u.tk) };
            unsafe { str_eq(&l.vname, &r.vname) && accessor_eq(&l.ta, &r.ta) }
        }
        TR_STRUCT => {
            let (l, r) = unsafe { (left.u.st, right.u.st) };
            if l.len != r.len {
                return false;
            }
            if l.len > 0
                && (l.keys.is_null() || r.keys.is_null() || l.types.is_null() || r.types.is_null())
            {
                return false;
            }
            (0..usize::from(l.len)).all(|i| unsafe {
                str_eq(&*l.keys.add(i), &*r.keys.add(i))
                    && type_ref_eq(&*l.types.add(i), &*r.types.add(i))
            })
        }
        _ => false,
    }
}

// =============================================================================
// Strides and words
// =============================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn user_stride_equal(
    left: *const CUserStride,
    right: *const CUserStride,
) -> bool {
    match unsafe { (left.as_ref(), right.as_ref()) } {
        (Some(l), Some(r)) => l == r,
        _ => false,
    }
}

// Returns the enum prim_t value of [text] or -1
#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_strtoprim(text: *const c_char, len: u32) -> i32 {
    unsafe { word(text, len) }
        .and_then(|w| w.parse::<Prim>().ok())
        .map_or(-1, |p| p.code().into())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn lex_is_keyword(text: *const c_char, len: u32) -> bool {
    unsafe { word(text, len) }.is_some_and(compiler::is_keyword)
}

// =============================================================================
// Types
// =============================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn type_byte_size(t: *const CType) -> u32 {
    unsafe { byte_size(t) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn type_equal(left: *const CType, right: *const CType) -> bool {
    unsafe { type_eq(left, right) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn type_snprintf(out: *mut c_char, size: u32, t: *const CType) -> i32 {
    let Ok(text) = unsafe { lift_type(t) }.map(|t| t.to_string()) else {
        return -1;
    };
    let Ok(len) = i32::try_from(text.len()) else {
        return -1;
    };

    if !out.is_null() && size > 0 {
        let n = text.len().min(size as usize - 1);
        unsafe {
            ptr::copy_nonoverlapping(text.as_ptr(), out.cast(), n);
            out.add(n).write(0);
        }
    }

    len
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_validate(t: *const CType, e: *mut NslangError) -> i32 {
    unsafe { status(e, || lift_type(t)?.validate()) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_tostr(
    t: *const CType,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> *mut c_char {
    unsafe {
        pointer(e, || {
            let text = lift_type(t)?.to_string();
            Ok(Arena::new(alloc)?.string(&text)?.data.cast_mut())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_copy(
    src: *const CType,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> *mut CType {
    unsafe {
        pointer(e, || {
            let a = Arena::new(alloc)?;
            a.put(lower_type(&a, &lift_type(src)?)?)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_serial_size(
    dest: *mut u16,
    t: *const CType,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let size = lift_type(t)?.serial_size()?;
            if let Some(dest) = dest.as_mut() {
                *dest = size;
            }
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_serialize(
    dest: *mut u8,
    cap: u32,
    t: *const CType,
    e: *mut NslangError,
) -> i32 {
    let mut written = 0;
    let code = unsafe {
        status(e, || {
            let t = lift_type(t)?;
            t.validate()?;

            let mut bytes = Vec::new();
            t.serialize(&mut bytes);
            if dest.is_null() || bytes.len() > cap as usize {
                return Err(Error::Arith(format!(
                    "serialized type needs {} bytes, only {cap} available",
                    bytes.len()
                )));
            }

            ptr::copy_nonoverlapping(bytes.as_ptr(), dest, bytes.len());
            written = bytes.len() as i32;
            Ok(())
        })
    };

    if code < 0 { code } else { written }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_deserialize(
    src: *const u8,
    len: u32,
    consumed: *mut u32,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> *mut CType {
    unsafe {
        pointer(e, || {
            let bytes = if src.is_null() {
                &[]
            } else {
                slice::from_raw_parts(src, len as usize)
            };
            let (t, used) = Type::deserialize(bytes)?;

            let a = Arena::new(alloc)?;
            let out = a.put(lower_type(&a, &t)?)?;
            if let Some(consumed) = consumed.as_mut() {
                *consumed = used as u32;
            }
            Ok(out)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_random(
    alloc: *const NslangAlloc,
    rng: *const NslangRng,
    depth: u32,
    max_size: u32,
    e: *mut NslangError,
) -> *mut CType {
    unsafe {
        pointer(e, || {
            let rng = rng.as_ref().ok_or(Error::InvalidArgument)?;
            let range = rng.range.ok_or(Error::InvalidArgument)?;
            // Don't trust the callback to stay in range
            let mut next = |lo: u32, hi: u32| range(rng.ctx, lo, hi).clamp(lo, hi);

            let t = random_type(&mut next, depth, max_size)?;
            let a = Arena::new(alloc)?;
            a.put(lower_type(&a, &t.view())?)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_print_data(
    buf: *const u8,
    t: *const CType,
    max_elems: u32,
    sink: *const NslangSink,
) {
    let Some(sink) = (unsafe { sink.as_ref() }) else {
        return;
    };
    let Some(write) = sink.write else {
        return;
    };

    let mut out = String::new();
    match unsafe { lift_type(t) } {
        Ok(t) if !buf.is_null() => {
            let data = unsafe { slice::from_raw_parts(buf, t.byte_size() as usize) };
            let _ = print_data(&mut out, data, &t, max_elems);
        }
        _ => out.push_str("<invalid type>\n"),
    }

    unsafe { write(sink.ctx, out.as_ptr().cast(), out.len()) };
}

// =============================================================================
// Type accessors, subtypes and type references
// =============================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_accessor_equal(
    left: *const CTypeAccessor,
    right: *const CTypeAccessor,
) -> bool {
    unsafe { accessor_eq(left, right) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn subtype_equal(left: *const CSubtype, right: *const CSubtype) -> bool {
    match unsafe { (left.as_ref(), right.as_ref()) } {
        (Some(l), Some(r)) => unsafe { str_eq(&l.vname, &r.vname) && accessor_eq(&l.ta, &r.ta) },
        _ => false,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_type_ref_equal(
    left: *const CTypeRef,
    right: *const CTypeRef,
) -> bool {
    match unsafe { (left.as_ref(), right.as_ref()) } {
        (Some(l), Some(r)) => unsafe { type_ref_eq(l, r) },
        _ => false,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_ta_subtype(
    reftype: *const CType,
    ta: *const CTypeAccessor,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> *mut CType {
    unsafe {
        pointer(e, || {
            let sub = lift_accessor(ta)?.subtype(&lift_type(reftype)?)?;
            let a = Arena::new(alloc)?;
            a.put(lower_type(&a, &sub)?)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_tr_construct(
    reftype: *const CType,
    tr: *const CTypeRef,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> *mut CType {
    unsafe {
        pointer(e, || {
            let built = lift_type_ref(tr)?.construct(&lift_type(reftype)?)?;
            let a = Arena::new(alloc)?;
            a.put(lower_type(&a, &built)?)
        })
    }
}

// =============================================================================
// Queries
// =============================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn query_equal(left: *const CQuery, right: *const CQuery) -> bool {
    let (Some(l), Some(r)) = (unsafe { left.as_ref() }, unsafe { right.as_ref() }) else {
        return false;
    };
    if l.tag != r.tag {
        return false;
    }

    unsafe {
        match l.tag {
            QT_READ | QT_WRITE | QT_REMOVE => {
                let (l, r) = (l.u.strided, r.u.strided);
                str_eq(&l.name, &r.name) && l.ustr == r.ustr
            }
            QT_INSERT => {
                let (l, r) = (l.u.insert, r.u.insert);
                str_eq(&l.name, &r.name) && l.ofst == r.ofst && l.len == r.len
            }
            QT_CREATE => {
                let (l, r) = (&l.u.create, &r.u.create);
                str_eq(&l.name, &r.name) && type_eq(&l.ty, &r.ty)
            }
            QT_DELETE | QT_GET => {
                let (l, r) = (l.u.named, r.u.named);
                str_eq(&l.name, &r.name) && (l.if_exists != 0) == (r.if_exists != 0)
            }
            QT_EXIT => true,
            QT_HELP => {
                let (l, r) = (l.u.help, r.u.help);
                match (l.has_command != 0, r.has_command != 0) {
                    (true, true) => l.command == r.command,
                    (false, false) => true,
                    _ => false,
                }
            }
            _ => false,
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn query_vname(dest: *mut CStr32, q: *const CQuery) -> bool {
    let (Some(dest), Some(q)) = (unsafe { dest.as_mut() }, unsafe { q.as_ref() }) else {
        return false;
    };

    *dest = match q.tag {
        QT_READ | QT_WRITE | QT_REMOVE => unsafe { q.u.strided.name },
        QT_INSERT => unsafe { q.u.insert.name },
        QT_CREATE => unsafe { q.u.create.name },
        QT_DELETE | QT_GET => unsafe { q.u.named.name },
        _ => return false,
    };
    true
}

// =============================================================================
// Compiler
// =============================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_type(
    dest: *mut CType,
    src: *const c_char,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let t = compiler::compile_type(text(src)?)?;
            t.validate()?;
            let lowered = lower_type(&Arena::new(alloc)?, &t)?;
            dest.as_mut()
                .ok_or(Error::InvalidArgument)?
                .clone_from(&lowered);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_subtype(
    dest: *mut CSubtype,
    src: *const c_char,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let st = compiler::compile_subtype(text(src)?)?;
            let lowered = lower_subtype(&Arena::new(alloc)?, &st)?;
            dest.as_mut()
                .ok_or(Error::InvalidArgument)?
                .clone_from(&lowered);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_type_ref(
    dest: *mut CTypeRef,
    src: *const c_char,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let tr = compiler::compile_type_ref(text(src)?)?;
            let lowered = lower_type_ref(&Arena::new(alloc)?, &tr)?;
            dest.as_mut()
                .ok_or(Error::InvalidArgument)?
                .clone_from(&lowered);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_query(
    dest: *mut CQuery,
    src: *const c_char,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let q = compiler::compile_query(text(src)?)?;
            let lowered = lower_query(&Arena::new(alloc)?, &q)?;
            dest.as_mut()
                .ok_or(Error::InvalidArgument)?
                .clone_from(&lowered);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_user_stride(
    dest: *mut CUserStride,
    src: *const c_char,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let stride = compiler::compile_user_stride(text(src)?)?;
            *dest.as_mut().ok_or(Error::InvalidArgument)? = lower_stride(&stride);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nslang_compile_multi_user_stride(
    dest: *mut CMultiUserStride,
    src: *const c_char,
    alloc: *const NslangAlloc,
    e: *mut NslangError,
) -> i32 {
    unsafe {
        status(e, || {
            let strides = compiler::compile_multi_user_stride(text(src)?)?;
            let lowered = CMultiUserStride {
                strides: lower_strides(&Arena::new(alloc)?, &strides)?,
                len: u32::try_from(strides.len()).map_err(|_| Error::NoMem)?,
            };
            dest.as_mut()
                .ok_or(Error::InvalidArgument)?
                .clone_from(&lowered);
            Ok(())
        })
    }
}

#[cfg(test)]
mod tests;
