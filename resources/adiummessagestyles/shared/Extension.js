var messages = [];
		
function deleteMessage(index) {
    var ael,ains,sib,add=0, ins, curins = document.getElementById('insert');
	ael = document.getElementById(messages[index]);
	ains = messages[index-1]? document.getElementById('old_insert_'+messages[index-1]) : 0;
	if (ains && ael.parentNode===ains.parentNode && messages[index+1]) {
		sib = document.getElementById(messages[index+1]);
		ins = document.getElementById('old_insert_'+messages[index]);
		if (sib && sib.parentNode===ins.parentNode) {
			messages.splice(index,1);
			ael.parentNode.replaceChild(sib,ael);
			return;
		}
	}
	messages.splice(index,1);
	ael.parentNode.removeChild(ael);
	if (index==messages.length && ains) {
		sib = ains.nextSibling;
		while (sib && sib.nodeType!=1) sib = sib.nextSibling;
		sib? sib.parentNode.insertBefore(curins,sib) : ains.parentNode.appendChild(curins);
	}
}

function appendCustumMessage(html,type,index,past) {
    var id = Math.round(Math.random()*10000)+'_'+Math.round(Math.random()*10000), 
        _ind = html.search(/(\<\w+\s+id\s*=\s*[\'\"]?insert[\'\"]?[^\>]*\>)/), 
        getSib = function(which,el){
            el = el[which+'Sibling'];
            while (el && el.nodeType!=1) el = el[which+'Sibling'];
            return el;
        }, 
        funcs = [
            function(nid,ind,ht,range,t){
                var ael,ains,newNode,sib,add=0, ins, curins;
                (curins = document.getElementById('insert') || {}).id = '_temp_insert_';
                ael = document.getElementById(messages[ind]);
                ains = document.getElementById('old_insert_'+messages[ind]);
                sib = messages[ind+1]? document.getElementById(messages[ind+1]) : 0;
                range.selectNode(ael.parentNode);
                newNode = range.createContextualFragment(ht);
                ains && (add = (sib && sib.parentNode===ains.parentNode));
                ael.parentNode.replaceChild(newNode,ael);
                ins = document.getElementById('insert');
                messages[ind] = nid;
                if (ins && sib) add? ins.parentNode.replaceChild(sib,ins) :  ins.parentNode && ins.parentNode.removeChild(ins);
                curins.id = 'insert';
            }, 
            function(nid,ind,ht,range,t){
                var ael,ains,newNode,sib,add=0, ins, curins;
                (curins = document.getElementById('insert') || {}).id = '_temp_insert_';
                ael = document.getElementById(messages[ind]);
                ains = messages[ind-1]? document.getElementById('old_insert_'+messages[ind-1]) : {parentNode:0};
                ind? messages.splice(ind,0,nid) : messages.unshift(nid);
                if (ains.parentNode===ael.parentNode) {
                    range.selectNode(ael.parentNode);
                    newNode = range.createContextualFragment(ht);
                    ael.parentNode.replaceChild(newNode,ael);
                    ins = document.getElementById('insert');
                    ins.parentNode.replaceChild(ael,ins);
                } else {
                    insertBef[ains.parentNode? t : 'appendMessage'](ael,ains,range,ht);
                    ins = document.getElementById('insert');
                    ins.parentNode.removeChild(ins);
                }
                curins.id = 'insert';
            }, 
            function(nid,ind,ht,range,t){
                this[1](nid,ind+1,ht,range,t);
            }
        ], 
        insertBef = {
            appendNextMessage: function(nel,ins,range,ht){
                range.selectNode(ins.parentNode);
                var sib = getSib('next',ins), newNode = range.createContextualFragment(ht);
                sib? sib.parentNode.insertBefore(newNode,sib) : ins.parentNode.appendChild(newNode);
            }, 
            appendMessage: function(nel,ins,range,ht){
                range.selectNode(nel.parentNode);
                nel.parentNode.insertBefore(range.createContextualFragment(ht),nel);
            }
        };
    
    _ind>-1 && (html = html.substring(0,_ind)+'<span style="display:none" id="old_insert_'+id+'"></span>'+html.substring(_ind));
    html = '<messagecont style="display:block;width:100%;" id="'+id+'">'+html+'</messagecont>';
	past==2 && !messages[index+1] && (index = -1);
	if (!messages[index]) messages.push(id)&&window[type](html);
	else {
        past = past||0;
		var shouldScroll = nearBottom();				
		funcs[past](id,index,html,document.createRange(),type);
		alignChat(shouldScroll);
	}
}		
