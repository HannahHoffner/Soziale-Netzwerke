#Aufgabe 2
import pandas as pd
import numpy as np
from scipy import stats
import matplotlib.pyplot as plt

# Daten einlesen
data = pd.read_csv('output.csv', names=['Time', 'Client', 'Bytes'])
# Entfernen des "Client-" Teils und Konvertierung in Integer
data['Client'] = data['Client'].str.replace('Client-', '').astype(int)

# Daten als Batch Means aggregieren
batch_means = data.groupby(np.arange(len(data)) // (300)).mean()


# Independent Replicates aggregieren
independent_replicates = [data[i:i+20].mean() for i in range(0, len(data), 20)]

# Konfidenzintervalle berechnen
conf_int_batch = stats.t.interval(0.95, len(batch_means)-1, loc=np.mean(batch_means), scale=stats.sem(batch_means))
conf_int_replicates = stats.t.interval(0.95, len(independent_replicates)-1, loc=np.mean(independent_replicates), scale=stats.sem(independent_replicates))


print(len(range(len(batch_means))))
print(len(conf_int_batch[0]))
print(len(conf_int_batch[1]))

# Stellen Sie sicher, dass alle drei Längen gleich sind, bevor Sie plt.fill_between aufrufen

# Ergebnisse plotten
plt.figure(figsize=(10,5))
plt.plot(batch_means, label='Batch Means')
plt.plot(independent_replicates, label='Independent Replicates')
plt.fill_between(range(len(batch_means)), conf_int_batch[0], conf_int_batch[1], color='b', alpha=0.2)
plt.fill_between(range(len(independent_replicates)), conf_int_replicates[0], conf_int_replicates[1], color='r', alpha=0.2)
plt.legend()
plt.show()


#Aufgabe 3
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Daten einlesen
data = pd.read_csv('output.csv', names=['Time', 'Client', 'Bytes'])
# Entfernen des "Client-" Teils und Konvertierung in Integer
data['Client'] = data['Client'].str.replace('Client-', '').astype(int)

# Filtern der Daten für Clients 3-5
filtered_data = data[data['Client'].isin([3, 4, 5])]

# Goodput für jeden Client berechnen
# Goodput = Gesamtzahl der Bytes dividiert durch die Gesamtzeit
goodput_data = filtered_data.groupby('Client')['Bytes'].sum() / (filtered_data['Time'].max() - filtered_data['Time'].min())
goodput_data = goodput_data / 1e6  # Konvertierung von Bytes/s in Mbit/s

# Plot für Goodput
plt.figure(figsize=(10, 5))
goodput_data.plot(kind='bar')
plt.title('Goodput der Clients 3-5 in Mbit/s')
plt.xlabel('Client')
plt.ylabel('Goodput (Mbit/s)')
plt.show()

# Übertragungszeit für 1 MB Datenpakete berechnen
# Hier wird angenommen, dass ein Paket genau 1 MB groß ist
# (vereinfachte Annahme und hängt von der Struktur der Daten ab)
transmission_time_data = filtered_data.groupby('Client')['Time'].apply(lambda x: x.diff().mean())

# Plot für Übertragungszeit
plt.figure(figsize=(10, 5))
transmission_time_data.plot(kind='bar')
plt.title('Durchschnittliche Übertragungszeit für 1 MB Datenpakete der Clients 3-5')
plt.xlabel('Client')
plt.ylabel('Zeit (s)')
plt.show()
