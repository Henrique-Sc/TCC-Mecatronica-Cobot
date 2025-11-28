// no topo do arquivo (após declarações iniciais)
import { openCommandConfig } from './block-configs.js';

let paletteHTML = null;
let draggedWrapper = null; // 🔴 NOVO: para realocar blocos já inseridos

const ARROW_SVG = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640">
  <path d="M439.1 297.4C451.6 309.9 451.6 330.2 439.1 342.7L279.1 502.7C266.6 515.2 246.3 515.2 233.8 502.7C221.3 490.2 221.3 469.9 233.8 457.4L371.2 320L233.9 182.6C221.4 170.1 221.4 149.8 233.9 137.3C246.4 124.8 266.7 124.8 279.2 137.3L439.2 297.3z"/>
</svg>`;

const programa = document.querySelector(".linha-programa");
const paletteCommands = document.querySelectorAll(".container-comandos .bloco-programa");
let placeholder = null;

export function initBlockDrag() {

  // ---------- Utils ----------
  function createPlaceholder() {
    if (placeholder) return placeholder;
    const box = document.createElement("div");
    box.classList.add("placeholder-box");
    const arrow = document.createElement("div");
    arrow.classList.add("arrow", "fbox", "center-v");
    arrow.innerHTML = ARROW_SVG;
    const wrapper = document.createElement("div");
    wrapper.classList.add("wrap-comando", "fbox", "center-v", "placeholder");
    wrapper.appendChild(box);
    wrapper.appendChild(arrow);
    placeholder = wrapper;
    return wrapper;
  }
  function removePlaceholder() {
    if (placeholder && placeholder.parentNode) placeholder.parentNode.removeChild(placeholder);
    placeholder = null;
  }
  function createArrow() {
    const arrow = document.createElement("div");
    arrow.classList.add("arrow", "fbox", "center-v");
    arrow.innerHTML = ARROW_SVG;
    return arrow;
  }


  // 1) Paleta arrastável
  paletteCommands.forEach(cmd => {
    cmd.setAttribute("draggable", "true");
    cmd.addEventListener("dragstart", e => {
      paletteHTML = cmd.outerHTML;
      draggedWrapper = null; // vem da paleta, não é realocação
      e.dataTransfer.setData("text/plain", "from-palette");
      e.dataTransfer.effectAllowed = "copy";
    });
  });


  [...programa.querySelectorAll(".wrap-comando")].forEach(makeExistingDraggable);

  // 3) Dragover (com placeholder)
  programa.addEventListener("dragover", e => {
    e.preventDefault();
    const startWrapper = programa.querySelector("#inicio-programa")?.closest(".wrap-comando");
    const endWrapper = programa.querySelector("#fim-programa")?.closest(".wrap-comando");
    const candidates = [...programa.querySelectorAll(".wrap-comando")]
      .filter(w => w !== startWrapper && w !== endWrapper && !w.classList.contains("placeholder"));
    let closest = { offset: Number.NEGATIVE_INFINITY, element: null };
    candidates.forEach(child => {
      const rect = child.getBoundingClientRect();
      const offset = e.clientX - (rect.left + rect.width / 2);
      if (offset < 0 && offset > closest.offset) {
        closest = { offset, element: child };
      }
    });
    const insertBeforeNode = closest.element || endWrapper;
    if (placeholder && placeholder.parentNode === programa) {
      const currentIndex = Array.from(programa.children).indexOf(placeholder);
      const targetIndex = Array.from(programa.children).indexOf(insertBeforeNode);
      if (currentIndex === targetIndex - 1 || programa.children[currentIndex] === insertBeforeNode) return;
      programa.removeChild(placeholder);
    }
    programa.insertBefore(createPlaceholder(), insertBeforeNode);
  });

  // 4) Drop
  programa.addEventListener("drop", e => {
    e.preventDefault();
    try {
      const startWrapper = programa.querySelector("#inicio-programa")?.closest(".wrap-comando");
      const endWrapper = programa.querySelector("#fim-programa")?.closest(".wrap-comando");
      if (!startWrapper || !endWrapper) return;

      let wrapper;
      if (draggedWrapper) {
        // 🔁 Reordenação: apenas move
        wrapper = draggedWrapper;
      } else if (paletteHTML) {
        // 🎨 Da paleta: cria novo
        const temp = document.createElement("div");
        temp.innerHTML = paletteHTML;
        let inner = temp.firstElementChild;
        if (!inner.classList.contains("bloco-programa")) {
          const div = document.createElement("div");
          div.classList.add("bloco-programa");
          div.appendChild(inner);
          inner = div;
        }
        // const wrapText = inner.querySelector('.wrap-text');
        // if (wrapText) {
        //   const t = wrapText.textContent.trim().toLowerCase();
        //   inner.dataset.command = t; // ex: 'mover', 'abrir'
        // }
        wrapper = document.createElement("div");
        wrapper.classList.add("wrap-comando", "fbox", "center-v");
        inner.classList.add("cmd-sequence");

        if (inner.dataset.command == 'garra') inner.innerHTML += `<div class="cmd-info">Abrir</div>`

        // if (Object.keys(inner.dataset).length != 0) {
        //   Object.assign(wrapper.dataset, inner.dataset);
        //   Object.keys(inner.dataset).forEach(key => {
        //     delete inner.dataset[key];
        //   });
        // }

        // inner.dataset.id = inner.dataset.id || `${blocoIdCounter++}`;
        wrapper.appendChild(inner);
        wrapper.appendChild(createArrow());
        makeExistingDraggable(wrapper); // novos blocos também podem ser movidos
      } else {
        return;
      }

      const insertBeforeNode = (placeholder && placeholder.parentNode === programa)
        ? placeholder
        : endWrapper;
      programa.insertBefore(wrapper, insertBeforeNode);
    } finally {
      removePlaceholder();
      paletteHTML = null;
      draggedWrapper = null;
    }

    needUpdateSequence = true;
  });

  // 5) Limpezas extras
  document.addEventListener("drop", removePlaceholder);
  document.addEventListener("dragend", removePlaceholder);
  window.addEventListener("keydown", e => {
    if (e.key === "Escape") {
      removePlaceholder();
      paletteHTML = null;
      draggedWrapper = null;
    }
  });

  [...programa.querySelectorAll('.wrap-comando')].forEach(w => {
    const inner = w.querySelector('.bloco-programa');
    if (inner && !inner.dataset.command) {
      const t = inner.querySelector('.wrap-text')?.textContent?.trim()?.toLowerCase();
      if (t) inner.dataset.command = t;
    }
  });
}

// 2) Blocos internos arrastáveis
function makeExistingDraggable(wrapper) {
  const startWrapper = programa.querySelector("#inicio-programa")?.closest(".wrap-comando");
  const endWrapper = programa.querySelector("#fim-programa")?.closest(".wrap-comando");
  const inner = wrapper.querySelector('.cmd-sequence')

  if (wrapper === startWrapper || wrapper === endWrapper) return; // não move início/fim


  wrapper.setAttribute("draggable", "true");
  wrapper.addEventListener("dragstart", e => {
    draggedWrapper = wrapper;
    paletteHTML = null; // não vem da paleta
    e.dataTransfer.setData("text/plain", "from-line");
    e.dataTransfer.effectAllowed = "move";
    setTimeout(() => wrapper.style.display = "none", 0); // esconde temporariamente
  });
  wrapper.addEventListener("dragend", () => {
    wrapper.style.display = ""; // reaparece
    draggedWrapper = null;
  });

  inner.addEventListener('click', e => {
    // evita abrir menu quando o clique começou como arraste
    if (e.which === 3) return; // ignora clique direito
    // evita abrir enquanto usuário estiver arrastando (dragstart define draggedWrapper)
    setTimeout(() => {
      // abre o menu posicionado próximo ao clique
      openCommandConfig(wrapper, e.clientX, e.clientY);
    }, 0);
  });


}

window.makeExistingDraggable = makeExistingDraggable