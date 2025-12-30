class Executer {
    EXPAND
    labelTLD = document.createElement("label")
    inputTLD = document.createElement("input")
    listTLD = document.createElement("select")
    labelIP = document.createElement("label")
    inputIP = document.createElement("input")

    constructor() {
        this.EXPAND = document.getElementById("execute-expand")
        this.labelTLD.innerHTML = "TLD: "
        this.labelIP.innerHTML = "IP: "
    }

    async prepare(action, parameters = []) {
        this.EXPAND.innerHTML = ""
        
        switch (action) {
            case "00TLD":
                console.log("you gonna claim a new TLD")

                this.EXPAND.appendChild(this.labelTLD)
                this.EXPAND.appendChild(this.inputTLD)
            break;

            case "00IP":
                console.log("you gonna claim a new IP")

                this.EXPAND.appendChild(this.labelTLD)
                this.EXPAND.appendChild(this.listTLD)
                this.EXPAND.appendChild(document.createElement("br"))
                this.EXPAND.appendChild(this.labelIP)
                this.EXPAND.appendChild(this.inputIP)
            break;
        
            default:
                break;
        }
    }
}