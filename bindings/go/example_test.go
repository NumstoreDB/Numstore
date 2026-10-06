package numstore_test

import (
	"fmt"
	"log"

	numstore "github.com/lincketheo/numstore-go"
)

func Example() {
	db, err := numstore.Open("my.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	tx, err := db.Begin()
	if err != nil {
		log.Fatal(err)
	}
	if err := db.Execute(tx, "create foo u32"); err != nil {
		tx.Rollback()
		log.Fatal(err)
	}
	if _, err := numstore.WriteFrom(db, tx, []uint32{1, 2, 3}, "insert foo 0 3"); err != nil {
		tx.Rollback()
		log.Fatal(err)
	}
	vals, err := numstore.ReadAllAs[uint32](db, tx, "read foo[0:3]")
	if err != nil {
		tx.Rollback()
		log.Fatal(err)
	}
	if err := tx.Commit(); err != nil {
		log.Fatal(err)
	}
	fmt.Println(vals)
}

func Example_smartFile() {
	f, err := numstore.OpenSmartFile("data.smf")
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	tx, _ := f.Begin()
	f.Insert(tx, numstore.AsBytes([]uint32{0, 1, 2, 3, 4, 5}), 0)
	numstore.SmartWrite(f, tx, []uint32{6, 7, 8}, 4, 2) // -> [0 6 2 7 4 8]
	out := make([]uint32, 3)
	numstore.SmartRead(f, tx, out, 4, 2) // -> [6 7 8]
	tx.Commit()
}
