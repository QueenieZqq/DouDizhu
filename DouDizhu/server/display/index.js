function textToArray(data) {
    data = data.replace("[","").replace("]", "").replace("\n", "")
    var result = []
    entries = data.split(")");
    for (let i in entries) {
        each_entry = entries[i].replace("(", "").split(", '");
        for (let j in each_entry)
            each_entry[j] = each_entry[j].replace("\'", "").replace(", ", "")
        result.push(each_entry)
    }
    return result

}


fetch('http://608dev-2.net/sandbox/sc/team018/week04/replay_handler.py?action=games')
.then(response => response.text())
.then(data => textToArray(data))
.then(data => {
    console.log(data);
    for (let i in data)  {
        if (i < data.length - 1) {
            let option = $("<option>").attr('id', data[i][0]).html(data[i][0]);
            console.log(data[i][0]);
            $("#games").append(option);
        }
    }

});

$("button").click(function(){
    
    let identity_url = "http://608dev-2.net/sandbox/sc/team018/week04/replay_handler.py?action=status&game_id=" + $('#games').val(); 
    let url = 'http://608dev-2.net/sandbox/sc/team018/week04/replay_handler.py?action=entries&game_id=' + $('#games').val(); 
    console.log(url);
    fetch(identity_url)
    .then(response => response.text())
    .then(data => {
        data = data.split(",");
        $("#ll").html(`Landlord: ${data[0]}`);
        $("#p1").html(`Peasant1: ${data[1]}`);
        $("#p2").html(`Peasant2: ${data[2]}`);
        console.log('hello');
    });
    fetch(url)
     .then(response => response.text())
     .then(data => textToArray(data))
     .then(data => {
        console.log(data);
        for (let i in data) {
            if (i < data.length - 1) {
                tr_entry = $("<tr>").attr('id', i);
                player = data[i][3];
                if (player =="") player = "No One Yet";
                timestamp = data[i][1];
                cards = data[i][2].split(",");
                let images = $("<div>")
                if (data[i][2]!="") {
                    for (let j in cards) {
                        let link = "http://web.mit.edu/sfang22/www/resources/" + cards[j] + '.jpg';
                        if (cards[j]=='OO') link = "http://web.mit.edu/sfang22/www/resources/O.jpg";
                        let image = $('<img>').attr('src', link).attr("height", "100").attr("width", 75);
                        images.append(image);
                    }
                }
                else {
                    images.html("Pass this round");
                }
                cards_landlord = data[i][5].split(",").length;
                if (data[i][5]=="") cards_landlord = 0;
                cards_peasant1 = data[i][6].split(",").length;     
                if (data[i][6]=="") cards_peasant1 = 0;      
                cards_peasant2 = data[i][7].split(",").length;
                if (data[i][7]=="") cards_peasant2 = 0;
                tr_entry.append($('<td>').html(player));
                tr_entry.append($('<td>').html(images));
                tr_entry.append($('<td>').html(`landlord: ${cards_landlord}, peasant1: ${cards_peasant1}, peasant2: ${cards_peasant2}`));
                tr_entry.append($('<td>').html(timestamp));
                $("tbody").append(tr_entry);
            }

        }
    });
  });

