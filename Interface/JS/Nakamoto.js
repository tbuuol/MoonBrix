class Nakamoto {
    IP
    PORT

    constructor(ip = "127.0.0.1", port = 10000) {
        this.IP = ip
        this.PORT = port
    }

    async Node(chain, command, parameters = []) {
        const url = "http://" + this.IP + ":" + this.PORT + "/"
        try {
            const response = await fetch(url, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    jsonrpc: "1.0",
                    id: chain,
                    method: command,
                    params: parameters
                })
            })
            //.then(response => response.json())
            //.then(data => console.log(data))
            //.catch(error => console.log(error))

            const data = response.json()
            return data
        } catch (error) {
            return error
        }
    }
}