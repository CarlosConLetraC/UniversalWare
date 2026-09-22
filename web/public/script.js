let recursoActual = 'marcas'; // tabla seleccionada por defecto al cargar el localhost. . .
let datosCargados = [];
let datosFiltrados = [];
let registroEnEdicion = null;

// Esquema DDL
const esquemaDDL = {
    marcas: {
        pk: ['id_marca'],
        fields: {
            id_marca: { type: 'pk' },
            nombre: { type: 'varchar', max: 100 },
            activa: { type: 'boolean', default: 1 }
        }
    },
    categorias: {
        pk: ['id_categoria'],
        fields: {
            id_categoria: { type: 'pk' },
            nombre: { type: 'varchar', max: 80 }
        }
    },
    productos: {
        pk: ['id_producto'],
        fields: {
            id_producto: { type: 'pk' },
            id_marca: { type: 'int', min: 1 },
            id_categoria: { type: 'int', min: 1 },
            nombre: { type: 'varchar', max: 120 },
            precio_venta: { type: 'decimal', step: '0.01', min: 0.01 }
        }
    },
    ingredientes: {
        pk: ['id_ingrediente'],
        fields: {
            id_ingrediente: { type: 'pk' },
            nombre: { type: 'varchar', max: 100 },
            unidad_medida: { type: 'enum', options: ['kg', 'gr', 'pieza'] },
            stock_actual: { type: 'decimal', step: '0.001', min: 0 },
            stock_minimo: { type: 'decimal', step: '0.001', min: 0 }
        }
    },
    recetas: {
        pk: ['id_producto', 'id_ingrediente'],
        fields: {
            id_producto: { type: 'int', min: 1, isPkPart: true },
            id_ingrediente: { type: 'int', min: 1, isPkPart: true },
            cantidad_requerida: { type: 'decimal', step: '0.001', min: 0.001 }
        }
    },
    clientes: {
        pk: ['id_cliente'],
        fields: {
            id_cliente: { type: 'pk' },
            nombre: { type: 'varchar', max: 100 },
            telefono: { type: 'varchar', max: 20 },
            direccion: { type: 'varchar', max: 255 }
        }
    },
    repartidores: {
        pk: ['id_repartidor'],
        fields: {
            id_repartidor: { type: 'pk' },
            nombre: { type: 'varchar', max: 100 },
            telefono: { type: 'varchar', max: 20 },
            vehiculo: { type: 'varchar', max: 50 }
        }
    },
    pedidos: {
        pk: ['id_pedido'],
        fields: {
            id_pedido: { type: 'pk' },
            id_cliente: { type: 'int', min: 1 },
            id_repartidor: { type: 'int', min: 1 },
            plataforma_origen: { type: 'enum', options: ['UberEats', 'Rappi', 'DidiFood', 'WebPropia'] },
            fecha_hora: { type: 'datetime' },
            estado: { type: 'enum', options: ['Pendiente', 'En Preparacion', 'En Camino', 'Entregado', 'Cancelado'] },
            total: { type: 'decimal', step: '0.01', min: 0 }
        }
    },
    detalle_pedidos: {
        pk: ['id_pedido', 'id_producto'],
        fields: {
            id_pedido: { type: 'int', min: 1, isPkPart: true },
            id_producto: { type: 'int', min: 1, isPkPart: true },
            cantidad: { type: 'int', min: 1 },
            precio_unitario: { type: 'decimal', step: '0.01', min: 0.01 }
        }
    }
};

// Fallback mock inicial para previsualización offline/standalone
const datosMockIniciales = {
    marcas: [
        { id_marca: 1, nombre: "Burger Republic (Dark)", activa: 1 },
        { id_marca: 2, nombre: "Tokyo Roll Virtual Lab", activa: 1 },
        { id_marca: 3, nombre: "Tacos Al Pastor 24/7", activa: 1 },
        { id_marca: 4, nombre: "Green Bowl Clean Food", activa: 0 }
    ],
    categorias: [
        { id_categoria: 1, nombre: "Hamburguesas Gourmet" },
        { id_categoria: 2, nombre: "Rolls & Sashimi" },
        { id_categoria: 3, nombre: "Bebidas Artesanales" },
        { id_categoria: 4, nombre: "Postres" }
    ],
    productos: [
        { id_producto: 101, id_marca: 1, id_categoria: 1, nombre: "Smash Burger Trufa Negra", precio_venta: 14.50 },
        { id_producto: 102, id_marca: 1, id_categoria: 1, nombre: "Double Bacon Cheddar Melt", precio_venta: 13.00 },
        { id_producto: 103, id_marca: 2, id_categoria: 2, nombre: "Spicy Tuna Crunch Roll (10pz)", precio_venta: 16.20 },
        { id_producto: 104, id_marca: 4, id_categoria: 1, nombre: "Quinoa Salmon Avocado Bowl", precio_venta: 12.80 }
    ],
    ingredientes: [
        { id_ingrediente: 1, nombre: "Carne Angus Madurada", unidad_medida: "kg", stock_actual: 45.500, stock_minimo: 15.000 },
        { id_ingrediente: 2, nombre: "Queso Cheddar Vintage", unidad_medida: "kg", stock_actual: 22.000, stock_minimo: 8.000 },
        { id_ingrediente: 3, nombre: "Pan Brioche Artesano", unidad_medida: "pieza", stock_actual: 180, stock_minimo: 50 },
        { id_ingrediente: 4, nombre: "Salmón Fresco Premium", unidad_medida: "kg", stock_actual: 12.300, stock_minimo: 5.000 }
    ],
    recetas: [
        { id_producto: 101, id_ingrediente: 1, cantidad_requerida: 0.180 },
        { id_producto: 101, id_ingrediente: 2, cantidad_requerida: 0.050 },
        { id_producto: 101, id_ingrediente: 3, cantidad_requerida: 1.000 }
    ],
    clientes: [
        { id_cliente: 1, nombre: "Elena Morales", telefono: "+34 622 112 334", direccion: "Calle Velázquez 42, 3B, Madrid" },
        { id_cliente: 2, nombre: "Carlos Santamaría", telefono: "+34 689 445 120", direccion: "Paseo de la Castellana 110, Madrid" },
        { id_cliente: 3, nombre: "Lucía Fernández", telefono: "+34 611 908 231", direccion: "Av. Diagonal 205, Barcelona" }
    ],
    repartidores: [
        { id_repartidor: 1, nombre: "Mateo Silva", telefono: "+34 655 019 283", vehiculo: "Moto Eléctrica Silence S01" },
        { id_repartidor: 2, nombre: "Javier Ortega", telefono: "+34 677 342 901", vehiculo: "Bicicleta Gravel" },
        { id_repartidor: 3, nombre: "Andrés Delgado", telefono: "+34 633 451 908", vehiculo: "Moto Honda PCX 125" }
    ],
    pedidos: [
        { id_pedido: 5001, id_cliente: 1, id_repartidor: 1, plataforma_origen: "UberEats", fecha_hora: "2025-02-28 21:15:00", estado: "En Camino", total: 42.00 },
        { id_pedido: 5002, id_cliente: 2, id_repartidor: 2, plataforma_origen: "Rappi", fecha_hora: "2025-02-28 21:28:00", estado: "En Preparacion", total: 29.50 },
        { id_pedido: 5003, id_cliente: 3, id_repartidor: 1, plataforma_origen: "WebPropia", fecha_hora: "2025-02-28 20:45:00", estado: "Entregado", total: 35.80 }
    ],
    detalle_pedidos: [
        { id_pedido: 5001, id_producto: 101, cantidad: 2, precio_unitario: 14.50 },
        { id_pedido: 5001, id_producto: 102, cantidad: 1, precio_unitario: 13.00 },
        { id_pedido: 5002, id_producto: 103, cantidad: 1, precio_unitario: 16.20 }
    ]
};

function seleccionarRecurso(recurso) {
    recursoActual = recurso;
    
    // Actualizar botones de navegación
    const botones = document.querySelectorAll('#nav-recursos button');
    botones.forEach(btn => {
        const esSeleccionado = btn.id === `btn-${recurso}`;
        btn.classList.toggle('btn-nav-active', esSeleccionado);
    });

    // Actualizar títulos
    const titulo = document.getElementById('titulo-tabla');
    const subtitulo = document.getElementById('subtitulo-tabla');
    const nombreLimpio = recurso.replace('_', ' ');
    if (titulo) titulo.textContent = `Vista de ${nombreLimpio.charAt(0).toUpperCase() + nombreLimpio.slice(1)}`;
    if (subtitulo) subtitulo.textContent = `Control administrativo y auditoría de la tabla '${recurso}' en base de datos.`;

    // Resetear búsqueda
    const searchInput = document.getElementById('input-busqueda');
    if (searchInput) searchInput.value = '';

    cargarTabla(recurso);
}

async function cargarTabla(recurso) {
    const thead = document.getElementById('tabla-head');
    const tbody = document.getElementById('tabla-body');
    const mensaje = document.getElementById('mensaje-estado');

    if (mensaje) {
        mensaje.style.display = "flex";
        mensaje.innerHTML = '<div class="spinner"></div><span>Consultando registros de ' + recurso + '...</span>';
    }
    if (thead) thead.innerHTML = "";
    if (tbody) tbody.innerHTML = "";

    try {
        const response = await fetch(`/api/${recurso}`);
        if (!response.ok) {
            const errData = await response.json().catch(() => ({}));
            throw new Error(errData.detalle || errData.error || `HTTP ${response.status}`);
        }
        datosCargados = await response.json();
    } catch (error) {
        console.warn(`[API WARNING] Error al conectar con /api/${recurso}: ${error.message}`);
        datosCargados = datosMockIniciales[recurso] || [];
    }

    datosFiltrados = [...datosCargados];
    renderizarFilas(datosFiltrados);
}

function renderizarFilas(lista) {
    const thead = document.getElementById('tabla-head');
    const tbody = document.getElementById('tabla-body');
    const mensaje = document.getElementById('mensaje-estado');
    const conteoLabel = document.getElementById('label-conteo-registros');
    const metricTotal = document.getElementById('metric-total');

    if (metricTotal) metricTotal.textContent = lista.length;
    if (conteoLabel) conteoLabel.textContent = `Mostrando ${lista.length} de ${datosCargados.length} registros`;

    if (!lista || lista.length === 0) {
        if (thead) thead.innerHTML = "";
        if (tbody) tbody.innerHTML = "";
        if (mensaje) {
            mensaje.style.display = "flex";
            mensaje.innerHTML = `
                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" style="opacity:0.6;"><circle cx="12" cy="12" r="10"/><line x1="8" y1="12" x2="16" y2="12"/></svg>
                <span>No hay registros para mostrar en esta vista.</span>
            `;
        }
        return;
    }

    if (mensaje) mensaje.style.display = "none";

    // 1. Obtener todas las columnas presentes en la respuesta
    const columnasOriginales = Object.keys(lista[0]);
    const recursoConf = esquemaDDL[recursoActual];
    
    // 2. Extraer las claves primarias según la configuración DDL
    const pkKeys = recursoConf ? recursoConf.pk : [];

    // 3. Reordenar las columnas: PKs primero, seguidas del resto
    const columnas = [
        ...pkKeys.filter(k => columnasOriginales.includes(k)),
        ...columnasOriginales.filter(k => !pkKeys.includes(k))
    ];

    // Encabezados de tabla
    if (thead) {
        thead.innerHTML = columnas.map(col => `<th>${col.replace('_', ' ')}</th>`).join('') 
            + `<th class="text-right">Acciones</th>`;
    }

    // Renderizado de celdas
    if (tbody) {
        tbody.innerHTML = lista.map((row, index) => {
            const celdas = columnas.map(col => {
                const val = row[col];
                const fieldDef = recursoConf?.fields?.[col];
                const esCampoFecha = (fieldDef && fieldDef.type === 'datetime') || 
                                     col.toLowerCase().includes('fecha') || 
                                     col.toLowerCase().includes('date');

                if (col === 'activa') {
                    return val === 1 || val === true || val === '1'
                        ? `<td><span class="badge-pill badge-active"><span style="width:6px; height:6px; background:#10b981; border-radius:50%;"></span> Activo</span></td>`
                        : `<td><span class="badge-pill badge-inactive"><span style="width:6px; height:6px; background:#ef4444; border-radius:50%;"></span> Inactivo</span></td>`;
                }
                if (col === 'plataforma_origen') {
                    return `<td><span class="badge-pill badge-platform">${val}</span></td>`;
                }
                if (col === 'estado') {
                    return `<td><span class="badge-pill" style="background:rgba(255,255,255,0.08);">${val}</span></td>`;
                }
                if (col.includes('precio') || col === 'total') {
                    return `<td class="price-cell">$${Number(val || 0).toFixed(2)}</td>`;
                }
                if (col.startsWith('id_')) {
                    return `<td class="code-cell">#${val}</td>`;
                }
                if (esCampoFecha) {
                    if (!val) return `<td>-</td>`;
                    const fechaIso = typeof val === 'string' ? val.replace(' ', 'T') : val;
                    const fechaObj = new Date(fechaIso);

                    if (isNaN(fechaObj.getTime())) {
                        return `<td class="code-cell">${val}</td>`;
                    }

                    const fechaFormateada = fechaObj.toLocaleString('es-MX', {
                        year: 'numeric',
                        month: '2-digit',
                        day: '2-digit',
                        hour: '2-digit',
                        minute: '2-digit',
                        hour12: true
                    });

                    return `<td class="code-cell">${fechaFormateada}</td>`;
                }
                return `<td>${val !== null && val !== undefined ? val : '-'}</td>`;
            }).join('');

            return `
                <tr>
                    ${celdas}
                    <td class="actions-cell">
                        <button onclick="abrirModalEditar(${index})" class="btn-action btn-action-edit">
                            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>
                            Editar
                        </button>
                        <button onclick="eliminarRegistro(${index})" class="btn-action btn-action-delete">
                            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/></svg>
                            Eliminar
                        </button>
                    </td>
                </tr>
            `;
        }).join('');
    }
}

function filtrarTablaEnVivo() {
    const query = document.getElementById('input-busqueda')?.value.toLowerCase().trim() || '';
    if (!query) {
        datosFiltrados = [...datosCargados];
    } else {
        datosFiltrados = datosCargados.filter(item => {
            return Object.values(item).some(val => 
                val !== null && String(val).toLowerCase().includes(query)
            );
        });
    }
    renderizarFilas(datosFiltrados);
}

function generarControlHTML(colName, colConfig, val = '') {
    const label = colName.replace('_', ' ');

    if (colConfig.type === 'pk' || colConfig.isPkPart) {
        const readonlyAttr = registroEnEdicion 
            ? 'readonly class="input-crud-readonly"' 
            : 'class="input-crud" required';
        return `
            <div class="form-group">
                <label>${label} (Clave Primaria)</label>
                <input type="number" name="${colName}" value="${val}" ${readonlyAttr}>
            </div>
        `;
    }

    if (colConfig.type === 'varchar') {
        return `
            <div class="form-group">
                <label>${label} <span style="opacity:0.6; font-weight:normal;">(Máx: ${colConfig.max} car.)</span></label>
                <input type="text" name="${colName}" maxlength="${colConfig.max}" value="${val}" class="input-crud" placeholder="Escriba ${label}..." required>
            </div>
        `;
    }

    if (colConfig.type === 'boolean') {
        const boolVal = (val === 1 || val === true || val === '1') ? '1' : '0';
        return `
            <div class="form-group">
                <label>${label}</label>
                <select name="${colName}" class="input-crud">
                    <option value="1" ${boolVal === '1' ? 'selected' : ''}>Activo (1)</option>
                    <option value="0" ${boolVal === '0' ? 'selected' : ''}>Inactivo (0)</option>
                </select>
            </div>
        `;
    }

    if (colConfig.type === 'enum') {
        return `
            <div class="form-group">
                <label>${label}</label>
                <select name="${colName}" class="input-crud">
                    ${colConfig.options.map(opt => `<option value="${opt}" ${val === opt ? 'selected' : ''}>${opt}</option>`).join('')}
                </select>
            </div>
        `;
    }

    if (colConfig.type === 'decimal' || colConfig.type === 'int') {
        const stepAttr = colConfig.step ? `step="${colConfig.step}"` : 'step="1"';
        const minAttr = colConfig.min !== undefined ? `min="${colConfig.min}"` : '';
        return `
            <div class="form-group">
                <label>${label}</label>
                <input type="number" name="${colName}" ${stepAttr} ${minAttr} value="${val}" class="input-crud" required>
            </div>
        `;
    }

    if (colConfig.type === 'datetime') {
        let dtValue = '';
        if (val) {
            dtValue = String(val).replace(' ', 'T').slice(0, 16);
        } else {
            const now = new Date();
            now.setMinutes(now.getMinutes() - now.getTimezoneOffset());
            dtValue = now.toISOString().slice(0, 16);
        }

        return `
            <div class="form-group">
                <label>${label}</label>
                <input type="datetime-local" name="${colName}" value="${dtValue}" class="input-crud" required>
            </div>
        `;
    }

    return `
        <div class="form-group">
            <label>${label}</label>
            <input type="text" name="${colName}" value="${val}" class="input-crud" required>
        </div>
    `;
}

function abrirModalCrear() {
    registroEnEdicion = null;
    document.getElementById('modal-titulo').innerHTML = `
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="var(--primary)" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M12 8v8M8 12h8"/></svg>
        Nuevo Registro en ${recursoActual}
    `;
    
    const recursoConf = esquemaDDL[recursoActual];
    const formFields = document.getElementById('form-fields');
    if (!formFields) return;
    formFields.innerHTML = '';

    for (const [colName, colConfig] of Object.entries(recursoConf.fields)) {
        if (colConfig.type === 'pk') continue;
        formFields.innerHTML += generarControlHTML(colName, colConfig, '');
    }

    document.getElementById('modal-form').classList.remove('modal-hidden');
}

function abrirModalEditar(index) {
    registroEnEdicion = datosFiltrados[index] || datosCargados[index];
    document.getElementById('modal-titulo').innerHTML = `
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#f59e0b" stroke-width="2"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>
        Editar Registro #${index + 1}
    `;
    
    const recursoConf = esquemaDDL[recursoActual];
    const formFields = document.getElementById('form-fields');
    if (!formFields) return;
    formFields.innerHTML = '';

    for (const [colName, colConfig] of Object.entries(recursoConf.fields)) {
        const val = registroEnEdicion[colName] !== undefined ? registroEnEdicion[colName] : '';
        formFields.innerHTML += generarControlHTML(colName, colConfig, val);
    }

    document.getElementById('modal-form').classList.remove('modal-hidden');
}

function cerrarModal() {
    document.getElementById('modal-form').classList.add('modal-hidden');
}

async function guardarRegistro(e) {
    e.preventDefault();
    const formData = new FormData(e.target);
    const payload = {};

    formData.forEach((value, key) => {
        const fieldConf = esquemaDDL[recursoActual].fields[key];
        if (fieldConf && fieldConf.type === 'boolean') {
            payload[key] = parseInt(value, 10);
        } else if (fieldConf && (fieldConf.type === 'decimal' || fieldConf.type === 'int')) {
            payload[key] = Number(value);
        } else if (fieldConf && fieldConf.type === 'datetime') {
            payload[key] = value ? value.replace('T', ' ') + (value.length === 16 ? ':00' : '') : '';
        } else {
            payload[key] = value;
        }
    });

    const esEdicion = registroEnEdicion !== null;
    const metodo = esEdicion ? 'PUT' : 'POST';

    try {
        const response = await fetch(`/api/${recursoActual}`, {
            method: metodo,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        if (!response.ok) throw new Error("Error en servidor backend");
        cerrarModal();
        cargarTabla(recursoActual);
    } catch (error) {
        if (esEdicion) {
            Object.assign(registroEnEdicion, payload);
        } else {
            const recursoConf = esquemaDDL[recursoActual];
            const pkField = recursoConf.pk[0];
            if (!payload[pkField]) {
                payload[pkField] = Math.floor(Math.random() * 9000) + 1000;
            }
            datosCargados.unshift(payload);
        }
        cerrarModal();
        filtrarTablaEnVivo();
    }
}

async function eliminarRegistro(index) {
    const reg = datosFiltrados[index] || datosCargados[index];
    const recursoConf = esquemaDDL[recursoActual];
    
    const params = new URLSearchParams();
    recursoConf.pk.forEach(pkKey => params.append(pkKey, reg[pkKey]));

    if (!confirm(`¿Estás seguro de que deseas eliminar este registro (${recursoConf.pk.map(k=>reg[k]).join(', ')})?`)) return;

    try {
        const response = await fetch(`/api/${recursoActual}?${params.toString()}`, {
            method: 'DELETE'
        });
        if (!response.ok) throw new Error("Error al eliminar");
        cargarTabla(recursoActual);
    } catch (error) {
        datosCargados = datosCargados.filter(item => item !== reg);
        filtrarTablaEnVivo();
    }
}

// Inicialización
window.onload = () => seleccionarRecurso('marcas');