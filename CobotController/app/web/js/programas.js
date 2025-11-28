import { openCommandConfig } from './block-configs.js';

let slots = [

];  // Aqui você pode iniciar carregando do Python

let editingSlot = null;
let confirmingAction = null;
let confirmingSlot = null;

async function updateSlots() {
    slots = await pywebview.api.load_slots()
    renderSlots()
}

// Renderizar a lista
function renderSlots() {
    const container = document.getElementById('slots');
    container.innerHTML = "";

    if (slots.length == 0) {
        container.innerHTML = '<div class="slot-vazio">Nenhum arquivo encontrado.</div>'
    }

    slots.forEach((slot, index) => {
        container.innerHTML += `
            <div class="slot" id="${slot.id}">
                <div class="slot-name">${slot.name}</div>
                <div class="slot-buttons">
                    <button class="btn-load" onclick="confirmLoad(${index})">Carregar</button>
                    <button class="btn-edit" onclick="openNameModal(${index})">Renomear</button>
                    <button class="btn-delete" onclick="confirmDelete(${index})">Excluir</button>
                </div>
            </div>
        `;
    });
}

/* -------------------------
      Modal de Nome
--------------------------*/

function openNameModal(index) {
    editingSlot = index;
    const modal = document.getElementById("modalNameBg");

    if (index === null) {
        document.getElementById("modalNameTitle").innerText = "Novo Programa";
        document.getElementById("programNameInput").value = "";
    } else {
        document.getElementById("modalNameTitle").innerText = "Renomear Programa";
        document.getElementById("programNameInput").value = slots[index].name;
    }

    modal.style.display = "flex";
}

function closeNameModal() {
    document.getElementById("modalNameBg").style.display = "none";
    document.getElementById("")
}

async function confirmName() {
    const name = document.getElementById("programNameInput").value.trim();

    if (!name) return;

    if (editingSlot === null) {
        // Criar novo slot
        // 👉 Aqui você chama Python:
        limparPrograma()
        await pywebview.api.clear_points();
        const data = generateObjectCommand();
        await pywebview.api.save_program(name, JSON.stringify(data))
        document.getElementById('container-slot-saves').classList.add('oculto');
        close_slot_save.style.display = 'inherit';

        await updateSlots();
        setProgram(slots[slots.length - 1]);

    } else {
        // Renomear slot
        // 👉 Aqui você pode chamar Python para atualizar o nome
        await pywebview.api.rename_slot(slots[editingSlot].id, name)
        await updateSlots();
    }

    closeNameModal();
}

/* -------------------------
   Modal de Confirmação
--------------------------*/

function confirmLoad(index) {
    confirmingAction = "load";
    confirmingSlot = index;
    if (!currentProgram) { doConfirmAction(); return; }
    document.getElementById("confirmText").innerText =
        "Carregar este programa? O atual será apagado.";

    document.getElementById("confirmBtn").onclick = doConfirmAction;
    document.getElementById("modalConfirmBg").style.display = "flex";
}

function confirmDelete(index) {
    confirmingAction = "delete";
    confirmingSlot = index;
    const modalConfirm = document.getElementById("modalConfirmBg");
    const confirmBtn = document.getElementById("confirmBtn")

    document.getElementById("confirmText").innerText =
        "Tem certeza que deseja excluir este programa?";

    confirmBtn.addEventListener('mouseenter', (e) => { e.target.classList.add("troll"); e.target.onmouseeenter = null });
    confirmBtn.onclick = doConfirmAction;
    confirmBtn.onanimationend = (e) => e.target.disabled = false;
    confirmBtn.disabled = true;
    modalConfirm.style.display = "flex";
    modalConfirm.classList.add("inverted-color-buttons")

}

function closeConfirmModal() {
    document.getElementById("modalConfirmBg").style.display = "none";
    document.getElementById("modalConfirmBg").classList.remove("inverted-color-buttons")
    document.getElementById("confirmBtn").classList.remove("troll")

}

async function doConfirmAction() {
    if (confirmingAction === "load") {
        // 👉 Aqui você carrega no Python:
        let data = await pywebview.api.load_program(slots[confirmingSlot].id)
        console.log(data);
        renderCommands(data)
        setPoints();
        document.getElementById('container-slot-saves').classList.add('oculto')
        close_slot_save.style.display = 'inherit'
        needUpdateSequence = true;

        // alert("Carregado: " + slots[confirmingSlot].name);
        setProgram(slots[confirmingSlot]);
    } else if (confirmingAction === "delete") {
        // 👉 Aqui você exclui no Python:
        await pywebview.api.delete_slot(slots[confirmingSlot].id)
        updateSlots();
    }

    closeConfirmModal();
}

function generateObjectCommand() {
    const comandos = document.querySelectorAll('.cmd-sequence');

    const data = {
        points: {},
        commands: []      // lista de comandos em ordem
    };

    // 1 — salvar os points atuais
    points.forEach((p, index) => {
        data.points[index] = p;
    });

    // 2 — percorrer comandos renderizados no HTML
    comandos.forEach(cmd => {
        const type = cmd.dataset.command;
        const params = JSON.parse(cmd.dataset.params || "{}");

        data.commands.push({
            type,
            params
        });
    });

    return data;
}


function buildCommandHTML(command, index) {
    const { type, params } = command;

    let iconInline = false;
    let icon = "";
    let label = "";
    let info = "";

    if (type === "mover") {
        iconInline = false;
        icon = params.mode === "Linear" ? "images/linear.svg" : "images/suavizamento.svg";
        label = "Mover";
        info = String(params.point + 1).padStart(3, "0");
    }
    else if (type === "garra") {
        iconInline = false;
        icon = params.acao == 0 ? "images/garra_aberta.svg" : "images/garra_fechada.svg";
        label = "Garra";
        info = params.acao == 0 ? "Abrir" : "Fechar";
    }
    else if (type === "delay") {
        iconInline = true;
        icon = `<svg class="cmd-icon icon-font-awesome" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640"><!--!Font Awesome Free v7.0.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free Copyright 2025 Fonticons, Inc.--><path d="M160 64C142.3 64 128 78.3 128 96C128 113.7 142.3 128 160 128L160 139C160 181.4 176.9 222.1 206.9 252.1L274.8 320L206.9 387.9C176.9 417.9 160 458.6 160 501L160 512C142.3 512 128 526.3 128 544C128 561.7 142.3 576 160 576L480 576C497.7 576 512 561.7 512 544C512 526.3 497.7 512 480 512L480 501C480 458.6 463.1 417.9 433.1 387.9L365.2 320L433.1 252.1C463.1 222.1 480 181.4 480 139L480 128C497.7 128 512 113.7 512 96C512 78.3 497.7 64 480 64L160 64zM416 501L416 512L224 512L224 501C224 475.5 234.1 451.1 252.1 433.1L320 365.2L387.9 433.1C405.9 451.1 416 475.5 416 501z"></path></svg>`;
        label = "Delay";
        info = String(params.delay).replace('.', ',') + "s";
    }

    return `
        <div class="wrap-comando fbox center-v" draggable="true">
            <div class="comandos bloco-programa fbox center-vh cmd-sequence"
                data-command="${type}"
                data-params='${JSON.stringify(params)}'
                draggable="true">

                <div class="wrap-svg">
                    ${!iconInline ? `<img src="${icon}" class="cmd-icon icons" draggable="false">` : icon}
                </div>

                <span class="cmd-text">${label}</span>
                <div class="cmd-info">${info}</div>
            </div>
            <div class="arrow fbox center-v">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640">
                    <path d="M439.1 297.4C451.6 309.9 451.6 330.2 439.1 342.7L279.1 502.7C266.6 515.2 246.3 515.2 233.8 502.7C221.3 490.2 221.3 469.9 233.8 457.4L371.2 320L233.9 182.6C221.4 170.1 221.4 149.8 233.9 137.3C246.4 124.8 266.7 124.8 279.2 137.3L439.2 297.3z"></path>
                </svg>
            </div>
        </div>
    `;
}

function htmlToElement(html) {
    const template = document.createElement("template");
    template.innerHTML = html.trim();
    return template.content.firstElementChild;
}

function renderCommands(data) {
    const container = document.querySelector(".linha-programa");
    const fimPrograma = document.getElementById('fim-programa') // Âncora

    // Limpar o DOM, mantendo apenas o #inicio-programa e #fim-programa, ambos .not-include
    // Eles são elementos fixos, para ficar na esquerda e na direita
    limparPrograma();    
    
    if (data) {
        data.commands.forEach((cmd, index) => {
            const cmdEl = htmlToElement(buildCommandHTML(cmd, index));
            container.insertBefore(cmdEl, fimPrograma);

            // Adicionando eventos de click para abrir o menu de configuração
            makeExistingDraggable(cmdEl)
            cmdEl.querySelector(".cmd-sequence").addEventListener("click", e => {
                // evita abrir menu quando o clique começou como arraste
                if (e.which === 3) return; // ignora clique direito
                // evita abrir enquanto usuário estiver arrastando (dragstart define draggedWrapper)
                setTimeout(() => {
                    // abre o menu posicionado próximo ao clique
                    openCommandConfig(cmdEl, e.clientX, e.clientY);
                }, 0);
            });
        });
    }
}

// Expor funções para o escopo global
window.updateSlots = updateSlots;
window.renderSlots = renderSlots;
window.confirmLoad = confirmLoad;
window.confirmDelete = confirmDelete;
window.openNameModal = openNameModal;
window.closeNameModal = closeNameModal;
window.closeConfirmModal = closeConfirmModal;
window.confirmName = confirmName;
window.generateObjectCommand = generateObjectCommand;