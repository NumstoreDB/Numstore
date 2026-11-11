// Given this invocation:
// word_enum! {
//     Color {
//         Red => "red",
//         Blue => "blue",
//     }
// }
//
// It expands into the following code:
//
// #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
// pub enum Color {
//     Red,
//     Blue,
// }
//
// impl Color {
//     pub const ALL: &'static [Color] = &[Color::Red, Color::Blue];
//
//     /// Returns the string literal associated with the variant.
//     pub const fn as_str(self) -> &'static str {
//         match self {
//             Color::Red => "red",
//             Color::Blue => "blue",
//         }
//     }
// }
//
// impl ::core::fmt::Display for Color {
//     fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
//         f.write_str(self.as_str())
//     }
// }
//
// impl ::core::str::FromStr for Color {
//     type Err = ();
//     fn from_str(s: &str) -> ::core::result::Result<Self, ()> {
//         match s {
//             "red" => Ok(Color::Red),
//             "blue" => Ok(Color::Blue),
//             _ => Err(()),
//         }
//     }
// }
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

        impl ::core::fmt::Display for $name {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                f.write_str(self.as_str())
            }
        }

        impl ::core::str::FromStr for $name {
            type Err = ();
            fn from_str(s: &str) -> ::core::result::Result<Self, ()> {
                match s { $($text => Ok($name::$var),)* _ => Err(()) }
            }
        }
    };
}
