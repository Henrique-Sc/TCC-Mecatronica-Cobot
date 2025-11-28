import { makeDraggable } from './dragModule.js';

export function openCommandConfig(wrapper, clientX, clientY) {
  // fecha menu anterior se houver
  closeCommandConfig();
  const blockCmd = wrapper.querySelector('.cmd-sequence')
  const cmdType = wrapper.querySelector('.bloco-programa').dataset.command

  const existing = blockCmd.dataset.params ? JSON.parse(blockCmd.dataset.params) : {};

  const menu = document.createElement('div');
  menu.id = 'cmd-config';
  menu.classList.add('cmd-config');

  // === Header ===
  const header = document.createElement('div');
  header.className = 'container-header fbox center-v';
  header.textContent = `Configurar: ${(cmdType || 'comando').toUpperCase()}`;
  menu.appendChild(header);
  header.innerHTML += `<svg id="close-cmd-config" class="close" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640"><!--!Font Awesome Free v7.0.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free Copyright 2025 Fonticons, Inc.-->
                <path d="M183.1 137.4C170.6 124.9 150.3 124.9 137.8 137.4C125.3 149.9 125.3 170.2 137.8 182.7L275.2 320L137.9 457.4C125.4 469.9 125.4 490.2 137.9 502.7C150.4 515.2 170.7 515.2 183.2 502.7L320.5 365.3L457.9 502.6C470.4 515.1 490.7 515.1 503.2 502.6C515.7 490.1 515.7 469.8 503.2 457.3L365.8 320L503.1 182.6C515.6 170.1 515.6 149.8 503.1 137.3C490.6 124.8 470.3 124.8 457.8 137.3L320.5 274.7L183.1 137.4z"></path>
            </svg>`

  // === Form ===
  const form = document.createElement('form');
  form.className = 'cmd-config-form';

  function field(labelText, inputEl) {
    const row = document.createElement('div');
    row.className = 'cmd-row';
    const label = document.createElement('label');
    label.textContent = labelText;
    row.appendChild(label);
    row.appendChild(inputEl);
    return row;
  }

  // Campos comuns
  if (cmdType.includes('mover')) {
    const position = document.createElement('select');
    position.name = 'point';
    if (points.length == 0) {
      position.disabled = true
      const opt = document.createElement('option')
      opt.value = "none"
      opt.textContent = "Sem posições"
      position.appendChild(opt)
    } else {
      points.forEach((p, i) => {
        const opt = document.createElement('option');
        opt.value = i; opt.textContent = i + 1;
        if (existing.point === i) opt.selected = true;
        position.appendChild(opt);
      })
    }
    form.appendChild(field('Posição', position));

    const mode = document.createElement('select');
    mode.name = 'mode';
    ['Linear', 'Suavizamento'].forEach(m => {
      const o = document.createElement('option');
      o.value = m; o.textContent = m;
      if (existing.mode === m) o.selected = true;
      mode.appendChild(o);
    });
    form.appendChild(field('Modo', mode));

    const speed = document.createElement('input');
    speed.type = 'number';
    speed.name = 'speed';
    speed.value = existing.speed ?? globalSpeed;
    speed.min = 10;
    speed.max = 100;
    form.appendChild(field('Velocidade', speed));

  } else if (cmdType.includes('garra')) {
    const acao = document.createElement('select');
    acao.name = "acao";

    ["Abrir", "Fechar"].forEach((e, i) => {
      const opt = document.createElement('option')
      opt.value = i
      opt.textContent = e
      if (existing.acao === i) opt.selected = true
      acao.appendChild(opt)
    })
    form.appendChild(field('Estado', acao));

  } else if (cmdType.includes('delay')) {
    const delay = document.createElement('input');
    delay.type = 'number';
    delay.step = '0.1';
    delay.name = 'delay';
    delay.value = existing.delay ?? 1;
    delay.min = 0.1;
    delay.max = 60;
    form.appendChild(field('Delay (s)', delay));

  }
  // else {
  //   const p = document.createElement('input');
  //   p.type='text';
  //   p.name='param';
  //   p.value = existing.param ?? '';
  //   form.appendChild(field('Parâmetro', p));
  // }


  // === Botões ===
  const actions = document.createElement('div');
  actions.className = 'cmd-actions';

  const saveBtn = document.createElement('button');
  saveBtn.type = 'button';
  saveBtn.textContent = 'Salvar';
  saveBtn.className = 'btn-save cursor-pointer';

  const cancelBtn = document.createElement('button');
  cancelBtn.type = 'button';
  cancelBtn.textContent = 'Cancelar';
  cancelBtn.className = 'btn-cancel cursor-pointer';

  const closeCmd = menu.querySelector('#close-cmd-config');

  const deleteBtn = document.createElement('button');
  deleteBtn.type = 'button';
  deleteBtn.textContent = 'Excluir';
  deleteBtn.className = 'btn-delete cursor-pointer';

  actions.appendChild(saveBtn);
  actions.appendChild(cancelBtn);
  actions.appendChild(deleteBtn);
  form.appendChild(actions);
  form.addEventListener('submit', e => e.preventDefault());
  menu.appendChild(form);
  document.body.appendChild(menu);

  // === Posiciona menu inicial ===
  const pad = 20;
  const vw = Math.max(document.documentElement.clientWidth || 0, window.innerWidth || 0);
  const vh = Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
  let left = clientX + pad;
  let top = clientY - pad;
  const rect = menu.getBoundingClientRect();
  if (left + rect.width + pad > vw) left = vw - rect.width - pad;
  if (top + rect.height + pad > vh) top = vh - rect.height - pad;
  menu.style.left = `${Math.max(pad, left)}px`;
  menu.style.top = `${Math.max(pad, top)}px`;
  menu.style.position = 'absolute'; // necessário para arrastar


  // === Validação Dinâmica ===
  function validateForm() {
    let valid = true;

    form.querySelectorAll('input, select').forEach(el => {
      el.classList.remove('input-error');
      if (el.disabled) return;

      if (el.type === 'number') {
        const value = el.value.trim();
        const num = parseFloat(value);
        if (value === '' || isNaN(num) ||
          (el.min && num < parseFloat(el.min)) ||
          (el.max && num > parseFloat(el.max))) {
          valid = false;
          el.classList.add('input-error');
        }
      }
    });

    const sequenceSelect = form.querySelector('select[name=position]');
    if (sequenceSelect && sequenceSelect.value === 'none') {
      valid = false; // força desativação
    }

    saveBtn.disabled = !valid;
    saveBtn.classList.toggle('btn-disabled', !valid);

    return valid;
  }


  // Revalida sempre que algo mudar
  form.addEventListener('input', validateForm);
  form.addEventListener('change', validateForm);

  // Valida inicialmente (caso venha com selects vazios etc)
  validateForm();

  // === Handlers ===
  closeCmd.addEventListener('click', () => doClose());
  cancelBtn.addEventListener('click', () => doClose());
  deleteBtn.addEventListener('click', () => { wrapper.remove(); doClose(); needUpdateSequence=true;});
  saveBtn.addEventListener('click', () => {
    if (!validateForm()) return; // impede salvar se inválido
    const data = {};
    new FormData(form).forEach((v, k) => {
      if (k == 'delay') {
        let aposVirgula = String(v).split('.')[1];
        if (aposVirgula && aposVirgula.length > 1) v = parseFloat(v).toFixed(1);
      }
      data[k] = (v === '' ? null : (isFinite(v) ? Number(v) : v));
    });

    blockCmd.dataset.params = JSON.stringify(data);


    // Atualiza rótulo
    const imgEl = blockCmd.querySelector('.cmd-icon');
    let cmdInfo = blockCmd.querySelector('.cmd-info');
    if (cmdInfo == null) {
      cmdInfo = document.createElement('div');
      cmdInfo.classList.add('cmd-info');
      blockCmd.appendChild(cmdInfo);
    }

    if (cmdType.includes('mover')) {
      cmdInfo.textContent = String(data.point +1).padStart(3, '0');
      imgEl.src = "images/" + (data.mode === "Linear" ? "linear.svg" : "suavizamento.svg");
    } else if (cmdType.includes('garra')) {
      cmdInfo.textContent = data.acao ? 'Fechar' : 'Abrir';
      imgEl.src = "images/garra_" + (data.acao == 0 ? 'aberta.svg' : 'fechada.svg');
    } else if (cmdType.includes('delay')) {
      cmdInfo.textContent = String(data.delay).replace('.', ',') + 's';
      // } else {
      //   textEl.textContent = `${textEl.textContent.split('\n')[0]} ${JSON.stringify(data)}`;
    }

    needUpdateSequence = true;
    doClose();
  });

  // setTimeout(()=> document.addEventListener('click', outsideClick),0);

  function outsideClick(e) {
    if (!menu.contains(e.target) && !wrapper.contains(e.target)) doClose();
  }

  function onKey(e) { if (e.key === 'Escape') doClose(); }
  document.addEventListener('keydown', onKey);

  function doClose() {
    if (menu && menu.parentNode) menu.parentNode.removeChild(menu);
    document.removeEventListener('click', outsideClick);
    document.removeEventListener('keydown', onKey);
  }

  // === Drag da janela ===
  let offsetX = 0, offsetY = 0;

  header.style.cursor = 'move';
  makeDraggable(menu, {
    handleSelector: '.container-header',
    limitTop: 0,
    limitLeft: 0,
    limitRight: 0,
    limitBottom: 0,
  })
  // header.addEventListener('mousedown', (e)=>{
  //   isDragging = true;
  //   offsetX = e.clientX - menu.offsetLeft;
  //   offsetY = e.clientY - menu.offsetTop;
  //   document.addEventListener('mousemove', onDrag);
  //   document.addEventListener('mouseup', stopDrag);
  // });

  // function onDrag(e){
  //   if (!isDragging) return;
  //   menu.style.left = (e.clientX - offsetX) + 'px';
  //   menu.style.top = (e.clientY - offsetY) + 'px';
  // }

  // function stopDrag(){
  //   isDragging = false;
  //   document.removeEventListener('mousemove', onDrag);
  //   document.removeEventListener('mouseup', stopDrag);
  // }

  return doClose;
}

// === fechamento global (exportado) ===
export function closeCommandConfig() {
  const m = document.getElementById('cmd-config');
  if (m) m.remove();
}
