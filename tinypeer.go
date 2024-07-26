package main

import (
	"fmt"
	"log"
	"math/rand"
	"net"

	"github.com/btcsuite/btcd/wire"
)

func main() {
	regtestNode := "127.0.0.1:18444"

	// establish tcp connection
	conn, err := net.Dial("tcp", regtestNode)
	if err != nil {
		log.Fatalf("could not establish connection to node: %v", err)
	}
	defer conn.Close()

	fmt.Println("connected to regtest node")

	me := wire.NewNetAddress(conn.LocalAddr().(*net.TCPAddr), wire.SFNodeNetwork)
	peer := wire.NewNetAddress(conn.RemoteAddr().(*net.TCPAddr), wire.SFNodeNetwork)
	nonce := rand.Uint64()
	versionMsg := wire.NewMsgVersion(me, peer, nonce, 1)

	err = wire.WriteMessage(conn, versionMsg, wire.ProtocolVersion, wire.TestNet)
	if err != nil {
		fmt.Printf("error sending version msg to peer: %v", err)
	}

	fmt.Println("wrote message to peer")

	msg, _, err := wire.ReadMessage(conn, wire.ProtocolVersion, wire.TestNet)
	if err != nil {
		fmt.Printf("error reading msg from peer: %v\n", err)
		return
	}

	fmt.Printf("msg received outside: %v\n", msg)
}
