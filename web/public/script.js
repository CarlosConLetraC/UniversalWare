let recursoActual = 'marcas';
let datosCargados = [];
let registroEnEdicion = null;

// Mapeo estricto del DDL para renderizar inputs específicos según el tipo de columna SQL
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

function seleccionarRecurso(recurso) {
    recursoActual = recurso;
    const botones = document.querySelectorAll('#nav-recursos button');
    botones.forEach(btn => {
        const esSeleccionado = btn.id === `btn-${recurso}`;
        btn.classList.toggle('btn-nav-active', esSeleccionado);
        btn.classList.toggle('btn-nav-inactive', !esSeleccionado);
    });
    cargarTabla(recurso);
}

async function cargarTabla(recurso) {
    const titulo = document.getElementById('titulo-tabla');
    const thead = document.getElementById('tabla-head');
    const tbody = document.getElementById('tabla-body');
    const mensaje = document.getElementById('mensaje-estado');

    titulo.textContent = `Vista de ${recurso.replace('_', ' ')}`;
    mensaje.textContent = "Cargando datos. . .";
    thead.innerHTML = "";
    tbody.innerHTML = "";

    try {
        const response = await fetch(`/api/${recurso}`);
        if (!response.ok) throw new Error("Error de conexión");
        
        datosCargados = await response.json();

        if (!datosCargados || datosCargados.length === 0) {
            mensaje.textContent = "No hay registros disponibles en esta tabla.";
            return;
        }

        mensaje.textContent = "";
        const columnas = Object.keys(datosCargados[0]);

        thead.innerHTML = columnas.map(col => `<th class="py-3 px-4 uppercase text-xs tracking-wider">${col}</th>`).join('') 
            + `<th class="py-3 px-4 uppercase text-xs tracking-wider text-right">Acciones</th>`;

        tbody.innerHTML = datosCargados.map((row, index) => `
            <tr>
                ${columnas.map(col => `<td>${row[col] !== null ? row[col] : ''}</td>`).join('')}
                <td class="text-right actions-cell">
                    <button onclick="abrirModalEditar(${index})" class="btn-action btn-action-edit">Editar</button>
                    <button onclick="eliminarRegistro(${index})" class="btn-action btn-action-delete">Eliminar</button>
                </td>
            </tr>
        `).join('');

    } catch (error) {
        console.error(error);
        mensaje.textContent = "Error al conectar con el servidor backend.";
        mensaje.className = "status-message";
    }
}

function generarControlHTML(colName, colConfig, val = '') {
    const label = colName.replace('_', ' ');

    if (colConfig.type === 'pk' || colConfig.isPkPart) {
        const readonlyAttr = registroEnEdicion 
            ? 'readonly class="input-crud-readonly"' 
            : 'class="input-crud" required';
        return `
            <div class="form-group">
                <label>${label}</label>
                <input type="number" name="${colName}" value="${val}" ${readonlyAttr}>
            </div>
        `;
    }

    if (colConfig.type === 'varchar') {
        return `
            <div class="form-group">
                <label>${label} (Max: ${colConfig.max})</label>
                <input type="text" name="${colName}" maxlength="${colConfig.max}" value="${val}" class="input-crud" required>
            </div>
        `;
    }

    if (colConfig.type === 'boolean') {
        const boolVal = (val === 1 || val === true || val === '1') ? '1' : '0';
        return `
            <div class="form-group">
                <label>${label}</label>
                <select name="${colName}" class="input-crud">
                    <option value="1" ${boolVal === '1' ? 'selected' : ''}>1 (Activo / Verdadero)</option>
                    <option value="0" ${boolVal === '0' ? 'selected' : ''}>0 (Inactivo / Falso)</option>
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
        const dtValue = val ? val.replace(' ', 'T') : '';
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
    document.getElementById('modal-titulo').textContent = `Nuevo Registro en ${recursoActual}`;
    
    const recursoConf = esquemaDDL[recursoActual];
    const formFields = document.getElementById('form-fields');
    formFields.innerHTML = '';

    for (const [colName, colConfig] of Object.entries(recursoConf.fields)) {
        if (colConfig.type === 'pk') continue;
        formFields.innerHTML += generarControlHTML(colName, colConfig, '');
    }

    document.getElementById('modal-form').classList.remove('modal-hidden');
}

function abrirModalEditar(index) {
    registroEnEdicion = datosCargados[index];
    document.getElementById('modal-titulo').textContent = `Editar Registro #${index + 1}`;
    
    const recursoConf = esquemaDDL[recursoActual];
    const formFields = document.getElementById('form-fields');
    formFields.innerHTML = '';

    for (const [colName, colConfig] of Object.entries(recursoConf.fields)) {
        const val = registroEnEdicion[colName] !== undefined ? registroEnEdicion[colName] : '';
        formFields.innerHTML += generarControlHTML(colName, colConfig, val);
    }

    // Corregido: se usa la clase nativa 'modal-hidden'
    document.getElementById('modal-form').classList.remove('modal-hidden');
}

function cerrarModal() {
    // Corregido: se usa la clase nativa 'modal-hidden'
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
            payload[key] = value.replace('T', ' ');
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

        if (!response.ok) throw new Error("Error en la operación del servidor.");

        cerrarModal();
        cargarTabla(recursoActual);
    } catch (error) {
        alert("Ocurrió un error al guardar el registro.");
        console.error(error);
    }
}

async function eliminarRegistro(index) {
    const reg = datosCargados[index];
    const recursoConf = esquemaDDL[recursoActual];
    
    const params = new URLSearchParams();
    recursoConf.pk.forEach(pkKey => params.append(pkKey, reg[pkKey]));

    if (!confirm(`¿Estás seguro de que deseas eliminar este registro?`)) return;

    try {
        const response = await fetch(`/api/${recursoActual}?${params.toString()}`, {
            method: 'DELETE'
        });

        if (!response.ok) throw new Error("Error al eliminar el registro.");

        cargarTabla(recursoActual);
    } catch (error) {
        alert("Ocurrió un error al eliminar el registro.");
        console.error(error);
    }
}

window.onload = () => seleccionarRecurso('marcas');