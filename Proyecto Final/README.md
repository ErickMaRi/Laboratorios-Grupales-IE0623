# Proyecto Final

El siguiente sirve para documentar el proyecto final del curso IE-0624 Laboratorio de Microcontroladores de la Universidad de Costa Rica. Que consta del diseño de un sistema de E-lettering usando internet de las cosas.

## Descripción 

![Diagrama del sistema plant UML](https://www.plantuml.com/plantuml/png/RLB1QXj13BtFLuWzjGTJw24KOqpYn8L2qvgQq8F9GPvHDyApeqKpOq9BFwha4_nZdUnipPPaJrgzfwSdZIvZGasvyoPnyWGtZArYX-38mvPZeX9lLFfCCKd9mfPEVz3pJxHyUWgmzcIJbeWj6XCF77ei0c2bwoLGIF3BGFbMQ0-jIWxkMhmeVERs8QryfegAEjAAtA0U1k2tCNsfmfQW6QqqezhkIbtc870Nv1uMYuwjYfMGL3mwqUKIAk_szEIo5rXlDn_UNrmyS-C3atU50IDdWo6Xz_WCqcISiyXAaWIAh-758zea2dSnex80HcZ6k85_u3fOr1PTXSz_ZJ6xaNiFizboqcaEwyVUYoBGaKAIE64kPhEfwJbytKVI8u97wCUqcMweYh_SnHokIxV7uHs_yqSsBob3uPUyfy5IE3BgkszaUSyA3eilU7Er-VJsjNPJXxSIaw82Xn-YNJqTSvvsu4yLQhDIuUeXvZgwsPrNLxRmdxEOSE_94bZq5Dp0lYn1kUo_)

Se propuso diseñar un sistema que realice la interfaz entre nuestra pantalla y la API de calendario para presentar el actual y siguientes eventos, con el nombre de los mismos y hora. Que muestre los distintos eventos de un calendario en un slideshow cada ciertos segundos.

Desplegando información sobre los detalles del evento como:
1. Hora y fecha
2. Nombre del dueño de la oficina o recipiente de la cita

Se diseña con el objetivo de colocarse en la entrada de una oficina, espacio de reuniones, aula, consultorio y otros espacios en los que es sensible el calendario a seguir sin interrupciones.

Dentro de los requerimientos mínimos para el cumplimiento del proyecto, se estableció a gracias a la retroalimentación el día de la propuesta:
1. Usar una pantalla con mayor resolución.
2. Añadir botones para ver las actividades futuras, además de retornar a la actual.
3. Diseñar una plataforma para que los encargados establezcan el calendario a utilizar.

Consideremos según nuestra propuesta las tres posibles acciones del usuario frente nuestro sistema:

![Diagrama de eventos plant UML](https://www.plantuml.com/plantuml/png/fLJDRYD54BxFKvJ21QGr4HmMY1IqzYG1Ix5bnNZXa4kbfsoQzC_GtME84G_3aGC7n2FunQZkQTipkqE2j3bCnbDVLttrrTVrnWTqeT6gy8_IrEXGWwUV6pA2uBS2-6_idFsbjCewk2AgbuzF4zLG6nB1kebr5RbXmArSqwqqrCKyl1T4yRXnjZ5L2_Xe_DLLUNMJCEcnO8fIo1fDUAPO3_PEoYjRRSWD8M-i2NFK15DTemX3uQNS4NpsnL2eJ_Ztei2e2AnyWqxQeZwXuMp1JrGApm0zhARzuDXLZJGsnMPbFpOZRPt4UtDh1QeMSZthOwwNYoy_V_qw1UVNtxsJV36zDoKgCbMabK3yhIWkBs5g68PAxb38_Jy6BY-BMGcZhsEM2oYjMSjDun2-bpUou6ynn6TJY4J0Wd_0g8Tt6a5mzDUOi7TcQHLN2yWUiEmna6rng0XeIoPOtqytIR1E60GvWOPV47wqJ8lrRb3b8b6M2nWhZO8CGZo21hlPZGq4TakkXKwQzPhVXNIYKUWYQvWVmFU6nICFJvp0ryt-JyylAbTCsirnIvkiMvfDL2_FFj9PSSiBHrvQPd6sP24bPwQpw1AvnK3H1gjfObwoEhJ_ctCys10J-WFyY6PGdWsJFubvGcRc1OTXb9FUKR0kYU0h0jrQ2LHa6vDbbkFKbDmnJlfQuLDHKRVigXrAxkq2PWtvu0PALzJjhQjt-QgbNJPeAWjfXccQEarJyLFl_t0RCXI51dKQeB2Qp2u3VMxRWMbeQncBqHXnTNNEl0Ahjh9AfJZrqJ-1T6qTAfr8Sq4FYdwAZlgqA505A2EbD6Yr_qlBG7qkQGbqBiZ045fl9s3N-z_m-tccSTx1z0m_RrlAi1JlMJkFwIIDZXuracAVjVKocR7WdycWIsc4i-PukHVa-7yb1KQdAcjhAB52-06qD4slOcRzba-i8rwWVf1CxUFfUZIp5N-aQXJpqoVfKDJkMtpAroDLj8RQXE-rUPsdeLCYWLMk_6A_HnI73zTcRaDBqSiiN2QHE7E_fqC3Ve-7F1WIvAEr4vQKfwXTRdgmYLGN3agqnqnrLFwE13Dnt5dqpzkM_RFheQNqltP3xk-_5iEayGBxKaHhPzLXtP2B3d_9qkb-dPoM-y7dZPtHvxdiiC7XeRVNXvn-HwHhXRlU4gWL1KmI3ssuViCVljlw-VgBxGsJQBJw5m00)

Donde anterior y siguiente siguen la misma lógica aproximadamente.

La cara del sistema vista por el encargado en su mayoría es una dependencia, pues usaremos la API de Google Calendar y el uso de Things Board para el monitoreo e ingreso de datos por parte de una persona encargada. 

## Diseño


## Componentes 

## Referencias
