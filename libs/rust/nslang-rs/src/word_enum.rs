/// Defines a fieldless enum whose variants map 1:1 to source words, with
/// `ALL`, `as_str`, `Display` and `FromStr` generated from the same table.
#[macro_export]
macro_rules! word_enum {
    ($(#[$m:meta])* $name:ident { $($var:ident => $text:literal),* $(,)? }) => {
        $(#[$m])*
        #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
        pub enum $name { $($var),* }

        impl $name {
            pub const ALL: &'static [$name] = &[$($name::$var),*];

            pub const fn as_str(self) -> &'static str {
                match self { $($name::$var => $text),* }
            }
        }

        impl fmt::Display for $name {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                f.write_str(self.as_str())
            }
        }

        impl FromStr for $name {
            type Err = ();
            fn from_str(s: &str) -> Result<Self, ()> {
                match s { $($text => Ok($name::$var),)* _ => Err(()) }
            }
        }
    };
}
