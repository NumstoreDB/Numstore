/// Types that can be copied byte-for-byte to and from numstore.
///
/// # Safety
/// Implementors must have no padding bytes, no pointers or references, and
/// every bit pattern must be a valid value. For your own row types use
/// `#[repr(C)]` and lay out fields so there is no padding:
///
/// ```ignore
/// #[repr(C)]
/// #[derive(Clone, Copy)]
/// struct Sample { t: u64, x: f32, y: f32 }   // 16 bytes, no padding
/// unsafe impl numstore::Element for Sample {}
/// ```
pub unsafe trait Element: Copy + 'static {}

macro_rules! impl_element {
    ($($t:ty),*) => { $(unsafe impl Element for $t {})* };
}
impl_element!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128, f32, f64);
unsafe impl<T: Element, const N: usize> Element for [T; N] {}

fn as_bytes<T: Element>(s: &[T]) -> &[u8] {
    // SAFETY: Element guarantees no padding and no invalid bit patterns.
    unsafe { std::slice::from_raw_parts(s.as_ptr().cast(), size_of_val(s)) }
}

fn as_bytes_mut<T: Element>(s: &mut [T]) -> &mut [u8] {
    // SAFETY: as above; any bytes written back form a valid T.
    unsafe { std::slice::from_raw_parts_mut(s.as_mut_ptr().cast(), size_of_val(s)) }
}

fn elem_size<T: Element>() -> Result<t_size> {
    t_size::try_from(size_of::<T>()).map_err(|_| Error::ElementTooLarge)
}

