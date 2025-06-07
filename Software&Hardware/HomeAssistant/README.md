## 1. Instalar o HACS no Home Assistant Container

### Passos:

1. **Entrar no container do Home Assistant:**

```bash
docker exec -it homeassistant bash
```

2. **Instalar o Git:**

```bash
apk add git
```

3. **Instalar o HACS:**

```bash
wget -q -O - https://install.hacs.xyz | bash -
```

4. **Reiniciar o Home Assistant:**

```bash
docker restart homeassistant
```

---

## 2. Configurar o HACS no Home Assistant

1. Vai à interface web do Home Assistant
2. Vai a **Definições > Dispositivos e Serviços**
3. Clica em **"+ Adicionar Integração"**, procura por `HACS` e segue os passos (autentica-te com GitHub)

---

## 3. Instalar "Mini Graph Card" via HACS

1. Vai a **HACS > Frontend**
2. Clica em **Explorar e instalar repositórios**
3. Procura por **"Mini Graph Card"** e instala
4. Reinicia o Home Assistant

---

## 4. Criar os Cards do Dashboard (YAML)

Vai a **Visão Geral > Três Pontos > Editar Dashboard > Três Pontos > Editar em YAML**, e cola os seguintes cards:

### 🌍 CO₂ (S88 + SGP30)

```yaml
type: custom:mini-graph-card
name: CO₂ (S88 + SGP30)
entities:
  - entity: sensor.s88_co2
    name: S88 CO₂
  - entity: sensor.sgp30_co2eq
    name: SGP30 CO₂eq
hours_to_show: 1
points_per_hour: 12
show:
  legend: true
  extrema: true
```

### 🌡️ Temperatura

```yaml
type: custom:mini-graph-card
name: Temperatura
entities:
  - entity: sensor.sen55_temperature
hours_to_show: 1
points_per_hour: 12
show:
  extrema: true
```

### 💧 Humidade Relativa

```yaml
type: custom:mini-graph-card
name: Humidade Relativa
entities:
  - entity: sensor.sen55_humidity
hours_to_show: 1
points_per_hour: 12
show:
  extrema: true
```

### 🌫️ Qualidade do Ar

```yaml
type: custom:mini-graph-card
name: Qualidade do Ar
entities:
  - entity: sensor.sgp30_tvoc
    name: TVOC
  - entity: sensor.sgp30_nox
    name: NOx
  - entity: sensor.sgp30_voc
    name: VOC
hours_to_show: 1
points_per_hour: 12
show:
  legend: true
  extrema: true
```

### 📈 Partículas (PM1.0, PM2.5, PM4.0, PM10)

```yaml
type: custom:mini-graph-card
name: Partículas em Suspensão
entities:
  - entity: sensor.sen55_pm1_0
    name: PM1.0
  - entity: sensor.sen55_pm2_5
    name: PM2.5
  - entity: sensor.sen55_pm4_0
    name: PM4.0
  - entity: sensor.sen55_pm10
    name: PM10
hours_to_show: 1
points_per_hour: 12
show:
  legend: true
  extrema: true
```

---