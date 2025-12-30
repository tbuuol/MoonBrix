const NAKAMOTO = new Nakamoto()
const EXECUTER = new Executer()

const SYNCSTART = document.getElementById("SyncStart")
const SYNCSTOP = document.getElementById("SyncStop")
const SHUTDOWN = document.getElementById("Shutdown")

const ACTION = document.getElementById("Action")

const FIRSTIP = "00000000"

ACTION.onchange = function(event) {
    console.log(event.target.value)
    EXECUTER.prepare(event.target.value)
}

Init()


function hexToText(hex) {
    let text = '';
    for (let i = 0; i < hex.length; i += 2) {
        text += String.fromCharCode(parseInt(hex.substr(i, 2), 16));
    }
    return text;
}


function hexToBlob(hex, mimeType = 'image/png') {
    const bytes = new Uint8Array(hex.length / 2);
    for (let i = 0; i < hex.length; i += 2) {
        bytes[i / 2] = parseInt(hex.substr(i, 2), 16);
    }
    return new Blob([bytes], { type: mimeType });
}


async function Init(){
    const response = await NAKAMOTO.Node("Kotia", "help", [])
    //console.log(response)

    if (response.hasOwnProperty("error") && response.error == null) {
        document.getElementById("KotiaCon").innerHTML = "TARDIS Connected"
        document.getElementById("KotiaCon").style.color = "green"
        document.getElementById("KotiaCon").style.fontWeight = "bold"

        SYNCSTART.disabled = false
        SHUTDOWN.disabled = false
    }

    ACTION.onchange({ target: ACTION })

    await Background()
    await AddressList()
}

async function AddressList() {
    const List = document.getElementById("address")

    const response = await NAKAMOTO.Node("Kotia", "listaddressgroupings", [])
    //console.log(response)

    if (response.result.length > 0) {
        //console.log(response.result[0])
        List.innerHTML = ""

        if (Array.isArray(response.result[0][0])) {
            // multi address Arrays
            for (let a = 0; a < response.result.length; a++) {
                for (let b = 0; b < response.result[a].length; b++) {
                    const element = response.result[a][b]
                    //console.log(element)

                    const option = document.createElement("option")
                    option.value = element[0]
                    
                    if (element[2] == "") option.innerHTML = element[0] + " - " + element[1]
                    else option.innerHTML = element[2] + " - " + element[1]

                    List.appendChild(option)
                }
            }
        } else {
            // single address Array
            for (let a = 0; a < response.result.length; a++) {
                const element = response.result[a]
                //console.log(element)

                const option = document.createElement("option")
                option.value = element[0]
                
                if (element[2] == "") option.innerHTML = element[0] + " - " + element[1]
                else option.innerHTML = element[2] + " - " + element[1]

                List.appendChild(option)
            }
        }
    }
}

async function Background() {
    const rawtx = await NAKAMOTO.Node("Kotia", "getrawtransaction", ["53dcdbe9c0d44d8e92cd04c9c8b62ba90644f8056a8d8c2a2742ad36c78a7e01", 1])
    let r = rawtx["result"]["vout"][0]["scriptPubKey"]["asm"].split(" ")[1]
    r = hexToText(r.substr(34, r.length -1))

    const Meta = JSON.parse(r)
    //console.log(Meta)

    let fullHEX = ""

    for (let a = 0; a < Meta["chunks"].length; a++) {
        const chunk = await NAKAMOTO.Node("Kotia", "getrawtransaction", [Meta["chunks"][a], 1])
        let c = chunk["result"]["vout"][0]["scriptPubKey"]["asm"].split(" ")[1]
        c = c.substr(32, c.length -1)
        fullHEX = fullHEX + c
    }

    //console.log(fullHEX)

    const blob = hexToBlob(fullHEX, Meta["type"])
    const url = URL.createObjectURL(blob)

    document.body.style.backgroundImage = `url(${url})`
    document.body.style.backgroundSize = "100% 100%";
    document.body.style.backgroundPosition = "center";
    document.body.style.backgroundRepeat = "no-repeat";
}

async function Shutdown() {
    const response = await NAKAMOTO.Node("MoonBrix", "Shutdown", [])
    console.log(response)
}

async function SyncStart() {
    SYNCSTART.disabled = true
    SHUTDOWN.disabled = true

    const response = await NAKAMOTO.Node("MoonBrix", "SyncStart", [])
    console.log(response)

    SYNCSTOP.disabled = false
}

async function SyncStop() {
    SYNCSTOP.disabled = true

    const response = await NAKAMOTO.Node("MoonBrix", "SyncStop", [])
    console.log(response)

    SYNCSTART.disabled = false
    SHUTDOWN.disabled = false
}

async function Get() {
    const response = await NAKAMOTO.Node("MoonBrix", "Get", [document.getElementById("searchWhat").value, document.getElementById("search").value])
    console.log(response)
}

function Execute() {
    const Address = document.getElementById("address").value
    console.log(Address)


}