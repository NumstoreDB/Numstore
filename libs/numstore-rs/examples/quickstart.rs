use numstore_rs::{Database, Result};

fn main() -> Result<()> {
    Database::cleanup("prices.db")?;
    let db = Database::open("prices.db")?;

    db.execute(None, "create prices f64")?;

    let numbers = [1.5f64, 2.25, 3.75];
    let bytes: Vec<u8> = numbers.iter().flat_map(|n| n.to_le_bytes()).collect();
    db.write(None, &bytes, "insert prices 0 3")?;

    // Insert one number in the middle
    db.write(None, &9.0f64.to_le_bytes(), "insert prices 1 1")?;

    let data = db.read_malloc(None, "read prices")?;
    for chunk in data.chunks_exact(8) {
        println!("{}", f64::from_le_bytes(chunk.try_into().unwrap()));
    }
    Ok(())
}
