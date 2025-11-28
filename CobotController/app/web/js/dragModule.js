// dragModule.js
export function makeDraggable(element, options = {}) {
  let {
    handleSelector = null,
    limitTop = 0,
    limitBottom = Infinity,
    limitLeft = -Infinity,
    limitRight = Infinity,
    safe = 0,
    observeResize = true,
  } = options;

  if (!element) return;

  // escolha do handle (se não informar, usa o próprio elemento)
  const handle = handleSelector ? element.querySelector(handleSelector) : element;
  if (!handle) return;
  handle.style.cursor = "move";

  let isDragging = false;
  let startX = 0, startY = 0;
  let initialLeft = 0, initialTop = 0;

  // retorna referência: se offsetParent posicionado -> usa ele, senão usa viewport (window)
  function getRef() {
    const parent = element.offsetParent;
    const parentIsPositioned = parent && getComputedStyle(parent).position !== "static";
    if (parent && parentIsPositioned) {
      const rect = parent.getBoundingClientRect();
      return {
        type: "parent",
        node: parent,
        left: rect.left,
        top: rect.top,
        width: parent.clientWidth,
        height: parent.clientHeight
      };
    } else {
      return {
        type: "window",
        node: window,
        left: 0,
        top: 0,
        width: window.innerWidth,
        height: window.innerHeight
      };
    }
  }

  // Computa os ranges permitidos RELATIVOS ao ref (left/top em px relativos ao ref.left/top)
  function computeRanges() {
    const ref = getRef();
    const refW = ref.width;
    const refH = ref.height;
    const ew = element.offsetWidth;
    const eh = element.offsetHeight;

    // Interpretação dos limits (conforme sua especificação):
    // - limit === 0 => encaixa na borda do ref
    // - limit > 0 => margem interna
    // - limit < 0 => margem externa (permite sair)
    // - Infinity => sem limite

    // intervalo mínimo (lado esquerdo)
    const minLeftFromLimit = isFinite(limitLeft) ? limitLeft : -Infinity;
    // intervalo máximo (lado direito) traduzido para 'left' max:
    // se limitRight é finito: left <= refW - ew - limitRight
    const maxLeftFromLimit = isFinite(limitRight) ? (refW - ew - limitRight) : Infinity;

    // analogo vertical
    const minTopFromLimit = isFinite(limitTop) ? limitTop : -Infinity;
    const maxTopFromLimit = isFinite(limitBottom) ? (refH - eh - limitBottom) : Infinity;

    // agora os limites impostos pelo safe:
    // queremos garantir pelo menos `safe` pixels visíveis do elemento:
    // left mínimo por safe  => left >= safe - ew  (se o elemento sair à esquerda, sua parte dentro = left+ew >= safe)
    const safeMinLeft = safe - ew;
    // left máximo por safe  => left <= refW - safe (se o elemento sair à direita, parte dentro = refW - left >= safe)
    const safeMaxLeft = refW - safe;

    const safeMinTop = safe - eh;
    const safeMaxTop = refH - safe;

    // intersecção entre limit-derived e safe-derived
    const allowedMinLeft = Math.max(minLeftFromLimit, safeMinLeft);
    const allowedMaxLeft = Math.min(maxLeftFromLimit, safeMaxLeft);
    const allowedMinTop = Math.max(minTopFromLimit, safeMinTop);
    const allowedMaxTop = Math.min(maxTopFromLimit, safeMaxTop);

    return {
      ref,
      ew, eh, refW, refH,
      allowedMinLeft, allowedMaxLeft,
      allowedMinTop, allowedMaxTop
    };
  }

  // converte viewport rect.left para posição relativa ao ref.left
  function toRelLeft(viewportLeft, refLeft) { return viewportLeft - refLeft; }
  function toRelTop(viewportTop, refTop) { return viewportTop - refTop; }

  // garante que o elemento esteja dentro do intervalo permitido (usado no resize)
  function clampAndKeepVisible() {
    const r = computeRanges();
    const rect = element.getBoundingClientRect();
    const curLeftRel = toRelLeft(rect.left, r.ref.left);
    const curTopRel = toRelTop(rect.top, r.ref.top);

    let newLeft = curLeftRel;
    let newTop = curTopRel;

    // se permitido infinito, Math.max/Math.min com Infinity funciona corretamente
    if (isFinite(r.allowedMinLeft) || isFinite(r.allowedMaxLeft)) {
      const lo = isFinite(r.allowedMinLeft) ? r.allowedMinLeft : -Infinity;
      const hi = isFinite(r.allowedMaxLeft) ? r.allowedMaxLeft : Infinity;
      // se intervalo invertido (lo > hi), força hi (evita crash)
      if (lo > hi) newLeft = Math.min(newLeft, hi);
      else newLeft = Math.max(lo, Math.min(newLeft, hi));
    }

    if (isFinite(r.allowedMinTop) || isFinite(r.allowedMaxTop)) {
      const lo = isFinite(r.allowedMinTop) ? r.allowedMinTop : -Infinity;
      const hi = isFinite(r.allowedMaxTop) ? r.allowedMaxTop : Infinity;
      if (lo > hi) newTop = Math.min(newTop, hi);
      else newTop = Math.max(lo, Math.min(newTop, hi));
    }

    // escreve no elemento (left/top relativos ao offsetParent/ref)
    element.style.position = element.style.position || "absolute";
    element.style.left = `${Math.round(newLeft)}px`;
    element.style.top  = `${Math.round(newTop)}px`;
  }

  // ---- eventos de arraste ----
  handle.addEventListener("mousedown", (ev) => {
    if (ev.button !== 0) return;
    if (ev.target.closest && ev.target.closest(".bloco-programa, .close")) return;

    ev.preventDefault();
    isDragging = true;

    const r = computeRanges();
    const refRect = (r.ref.type === "parent") ? r.ref.node.getBoundingClientRect() : { left: 0, top: 0 };

    const rect = element.getBoundingClientRect();
    initialLeft = toRelLeft(rect.left, refRect.left);
    initialTop  = toRelTop(rect.top, refRect.top);

    startX = ev.clientX;
    startY = ev.clientY;

    Object.assign(element.style, {
      position: "absolute",
      left: `${Math.round(initialLeft)}px`,
      top: `${Math.round(initialTop)}px`,
      bottom: "unset"
    });

    document.addEventListener("mousemove", onMouseMove);
    document.addEventListener("mouseup", onMouseUp);
  });

  function onMouseMove(ev) {
    if (!isDragging) return;

    const dx = ev.clientX - startX;
    const dy = ev.clientY - startY;

    const r = computeRanges();

    let newLeft = initialLeft + dx;
    let newTop  = initialTop + dy;

    // clamp horizontal
    if (isFinite(r.allowedMinLeft) || isFinite(r.allowedMaxLeft)) {
      const lo = isFinite(r.allowedMinLeft) ? r.allowedMinLeft : -Infinity;
      const hi = isFinite(r.allowedMaxLeft) ? r.allowedMaxLeft : Infinity;
      newLeft = Math.max(lo, Math.min(newLeft, hi));
    }
    // clamp vertical
    if (isFinite(r.allowedMinTop) || isFinite(r.allowedMaxTop)) {
      const lo = isFinite(r.allowedMinTop) ? r.allowedMinTop : -Infinity;
      const hi = isFinite(r.allowedMaxTop) ? r.allowedMaxTop : Infinity;
      newTop = Math.max(lo, Math.min(newTop, hi));
    }

    element.style.left = `${Math.round(newLeft)}px`;
    element.style.top  = `${Math.round(newTop)}px`;
  }

  function onMouseUp() {
    isDragging = false;
    document.removeEventListener("mousemove", onMouseMove);
    document.removeEventListener("mouseup", onMouseUp);
  }

  // ---- resize handling ----
  let parentObserver = null;
  const initialRef = computeRanges().ref;
  if (observeResize) {
    if (initialRef.type === "parent" && typeof ResizeObserver !== "undefined") {
      try {
        parentObserver = new ResizeObserver(() => clampAndKeepVisible());
        parentObserver.observe(initialRef.node);
      } catch (err) {
        window.addEventListener("resize", clampAndKeepVisible);
      }
    } else {
      window.addEventListener("resize", clampAndKeepVisible);
    }
  }

  // chamada inicial para ajustar caso já esteja fora
  clampAndKeepVisible();

  // retorna API para cleanup se necessário
  return {
    destroy() {
      // nota: handler anônimo mousedown não é removível aqui sem guardar a função, mas isso geralmente não é necessário
      document.removeEventListener("mousemove", onMouseMove);
      document.removeEventListener("mouseup", onMouseUp);
      window.removeEventListener("resize", clampAndKeepVisible);
      if (parentObserver) parentObserver.disconnect();
    }
  };
}
